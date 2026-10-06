import { createProgressAnnouncer, experimentText, fetchPacked, integrityUnavailableMessage, physicalValues, type CatalogHit, type CatalogRecord, type Manifest } from './catalog';
import { downloadText, errorMessage } from './browser';
type Density = Record<string,number[][]>;
type Kernel = {catalog_coordinate:(...v:number[])=>number; catalog_period_days:(q:number,e:number)=>number; _initialize?:()=>void};
type SearchReply = {type:'search'; request:number; done:boolean; hits?:CatalogHit[]; total?:number; waiting?:boolean; error?:string;
  cancelled?:boolean; needsFullScan?:{bytes:number; files:number}; source?:string};
type BuildReply = {type:'build'; state:'progress'|'done'|'cancelled'|'error'; files:number; totalFiles:number; bytes:number; totalBytes:number; error?:string};
type RecordReply = {type:'record'; request:number; record?:CatalogRecord; error?:string};

export async function mountCatalog(root: HTMLElement): Promise<void> {
  const get = <T extends HTMLElement>(selector:string) => root.querySelector<T>(selector)!;
  const status = get('[data-search-status]'), selectionStatus = get('[data-selection-status]');
  const dialog = get<HTMLDialogElement>('[data-object-dialog]'), basketStatus = get('[data-basket-status]');
  const consent = get('[data-scan-consent]'), confirmScan = get<HTMLButtonElement>('[data-scan-confirm]');
  const dismissScan = get<HTMLButtonElement>('[data-scan-dismiss]'), stopScan = get<HTMLButtonElement>('[data-scan-stop]');
  // Full-index download status lives in its own block: a <progress> bar and
  // precise text that are not live, plus a polite region that hears only
  // throttled milestones (createProgressAnnouncer).
  const scanProgress = get('[data-scan-progress]'), scanBar = get<HTMLProgressElement>('[data-scan-progress-bar]');
  const scanText = get('[data-scan-progress-text]'), scanAnnounce = get('[data-scan-announce]');
  let announceProgress = createProgressAnnouncer();
  let inspectedId: number | undefined, shownResults = '';
  const form = get<HTMLFormElement>('[data-catalog-search]');
  const group = form.elements.namedItem('group') as HTMLSelectElement;
  const queryInput = form.querySelector<HTMLInputElement>('input[type="search"]')!;
  const canvas = get<HTMLCanvasElement>('[data-density]'), context = canvas.getContext('2d')!;
  const base = root.dataset.base!, catalogBase = `${base}catalog/`;
  const basket = new Map<number,CatalogRecord>();
  let selected: CatalogRecord | undefined, manifest: Manifest, density: Density, kernel: Kernel, worker: Worker;
  let page = 0, request = 0, selectionRequest = 0;
  // Set once the user accepts the full-index download; it is never implied.
  let fullScanAccepted = false;
  let path: number[][] = [], point: number[] | undefined;
  const previous = get<HTMLButtonElement>('[data-previous]'), next = get<HTMLButtonElement>('[data-next]');
  const add = get<HTMLButtonElement>('[data-add]'), prepare = get<HTMLButtonElement>('[data-prepare]'), download = get<HTMLButtonElement>('[data-export]');
  const number = (v:number) => v.toLocaleString('en-US');
  const logPoint = (x:number,z:number):number[] => {
    const r = Math.hypot(x,z), scale = r ? Math.log10(1+r)*60/r : 0;
    return [x*scale,z*scale];
  };
  const draw = () => {
    if (!density) return;
    const cells = Object.entries(density).filter(([cls]) => !group.value || cls === group.value).flatMap(([,v])=>v);
    let bound = 100;
    for (const p of [...cells,...path,...(point ? [point] : [])]) bound = Math.max(bound,Math.hypot(p[0],p[1]));
    const scale = 250/bound;
    context.fillStyle = '#142033'; context.fillRect(0,0,800,600);
    context.strokeStyle = '#516073'; context.lineWidth=1;
    context.fillStyle = '#ddd2b6'; context.font='14px monospace';
    for (const au of [1,3,10,30,100,1000]) {
      const radius = Math.log10(1+au)*60*scale;
      if (radius > 260) continue;
      context.beginPath(); context.arc(400,300,radius,0,Math.PI*2); context.stroke();
      context.fillText(`${au} AU`,404+radius,295);
    }
    for (const [x,z,count] of cells) {
      context.fillStyle=`rgba(197,173,112,${Math.min(.95,.15+Math.log10(1+count)/5)})`;
      context.fillRect(400+x*scale,300-z*scale,Math.max(1,scale),Math.max(1,scale));
    }
    context.strokeStyle='#94d9e0'; context.lineWidth=2; context.beginPath();
    path.forEach(([x,z],i) => i ? context.lineTo(400+x*scale,300-z*scale) : context.moveTo(400+x*scale,300-z*scale)); context.stroke();
    if (point) {context.fillStyle='#fff'; context.beginPath(); context.arc(400+point[0]*scale,300-point[1]*scale,5,0,Math.PI*2); context.fill();}
    context.fillStyle='#f4d47c'; context.beginPath(); context.arc(400,300,4,0,Math.PI*2); context.fill();
    get('[data-map-status]').textContent = `${number(cells.reduce((n,p)=>n+p[2],0))} mapped records in ${number(cells.length)} displayed density cells. ${selected ? selected[1]+' highlighted.' : ''}`;
  };
  const updateBasket = () => {
    get('[data-basket-count]').textContent=`${basket.size} / 16`;
    get('[data-basket]').replaceChildren(...Array.from(basket.values(),r => {
      const li=document.createElement('li'), remove=document.createElement('button');
      li.append(document.createTextNode(r[1]+' ')); remove.textContent='Remove'; remove.setAttribute('aria-label',`Remove ${r[1]}`);
      remove.onclick=()=>{basket.delete(r[0]);updateBasket();basketStatus.textContent=`${r[1]} removed. ${basket.size} bodies selected.`;get<HTMLElement>('[data-basket-summary]').focus();}; li.append(remove); return li;
    }));
    prepare.disabled=download.disabled=basket.size===0;
    add.disabled=!selected || basket.has(selected[0]) || basket.size>=16;
  };
  // Record lookups run in the worker (with its small data-shard cache); only
  // the newest request for the open dialog is honored.
  const pendingRecords = new Map<number,{resolve:(r:CatalogRecord)=>void; reject:(e:Error)=>void}>();
  const requestRecord = (id:number, orbitClass:string, token:number) => new Promise<CatalogRecord>((resolve,reject) => {
    // The worker answers only the newest lookup; settle superseded ones now.
    pendingRecords.forEach(pending=>pending.reject(new Error('Superseded lookup.')));pendingRecords.clear();
    pendingRecords.set(token,{resolve,reject});
    worker.postMessage({type:'record',base:catalogBase,manifest,id,group:orbitClass,request:token});
  });
  const inspect = async (id:number, orbitClass:string) => {
    inspectedId = id;
    get('[data-object-title]').textContent = 'Loading object…';
    get('[data-object-details]').replaceChildren(); get('[data-object-source]').replaceChildren();
    if (!dialog.open) dialog.showModal();
    const token=++selectionRequest;
    selected=undefined; add.disabled=true; selectionStatus.textContent='Loading source record…';
    try {
      const r=await requestRecord(id,orbitClass,token);
      if (token!==selectionRequest) return;
      selected=r; const p=physicalValues(r);
      get('[data-object-title]').textContent=r[1];
      const quality=(q:number)=>q===0?'measured':q===1?'estimated':q===2?'unknown':'published; quality unclassified';
      const dwarf=[20000001,20134340,20136199,20136472,20136108].includes(id);
      const values: [string,string][]=[['Identity',`${r[0]} / ${r[2]}`],['Classification',`${dwarf?'Dwarf planet · ':''}${manifest.classNames[r[3]]??r[3]}`],
        ['Source epoch',`JD ${r[4]} TDB`],['Perihelion',`${r[5]} AU`],['Eccentricity',String(r[6])],['Inclination',`${r[7]}°`],
        ['Orbit condition code',r[13]??'Unavailable'],['Mass',p.massQuality===2?'Unknown (test particle)':`${p.massKg.toExponential(6)} kg (${quality(p.massQuality)})`],
        ['Mean/effective radius',p.radiusQuality===2?'Unknown (marker only)':`${p.radiusM/1000} km (${quality(p.radiusQuality)})`]];
      get('[data-object-details]').replaceChildren(...values.flatMap(([label,value])=>{const dt=document.createElement('dt'),dd=document.createElement('dd');dt.textContent=label;dd.textContent=value;return [dt,dd];}));
      const source=document.createElement('a'); source.href=`https://ssd.jpl.nasa.gov/tools/sbdb_lookup.html#/?sstr=${encodeURIComponent(r[2])}`; source.textContent='JPL source record';
      const supplemental=document.createElement('a'); supplemental.href='https://ssd.jpl.nasa.gov/planets/phys_par.html'; supplemental.textContent='Dwarf-planet physical references';
      get('[data-object-source]').replaceChildren(source,...(dwarf?[document.createTextNode(' · '),supplemental]:[]));
      path=[]; point=undefined;
      experimentText([r],manifest.epoch); // Same availability boundary as the experiment controls.
      const period=kernel.catalog_period_days(r[5]!,r[6]!);
      // Ecliptic X/Y in AU: simulation x is ecliptic X and simulation z is ecliptic -Y.
      const position=(date:number)=>[0,2].map(component=>(component===2?-1:1)*kernel.catalog_coordinate(r[5]!,r[6]!,r[7]!,r[8]!,r[9]!,r[10]!,date,component)/149597870700);
      for (let i=0;i<=256;++i) {
        const date=r[10]!+(period ? period*i/256 : (i/256-.5)*73050);
        const [x,z]=position(date); if (!Number.isFinite(x+z)) throw new Error('Orbit preview unavailable.');
        path.push(logPoint(x,z));
      }
      const [x,z]=position(manifest.epoch); point=logPoint(x,z);
      selectionStatus.textContent='Source orbit ready. Add this body to your experiment basket.';
      updateBasket();draw();
    } catch(error) {
      if(token===selectionRequest) {selectionStatus.textContent=errorMessage(error);add.disabled=true;path=[];point=undefined;draw();}
    }
  };
  get('[data-close-object]').onclick = () => dialog.close();
  dialog.addEventListener('close', () => {
    // Search streams can replace a result while its source record is loading.
    // Resolve the current control instead of restoring a detached old node.
    const result = root.querySelector<HTMLButtonElement>(`[data-catalog-id="${inspectedId}"]`);
    (result ?? queryInput)?.focus();
  });
  add.onclick=()=>{if(selected&&basket.size<16){basket.set(selected[0],selected);updateBasket();selectionStatus.textContent=`${selected[1]} added. ${basket.size} / 16 bodies selected. Return to results to choose another world, or open the experiment basket.`;}};
  prepare.onclick=()=>{
    try {
      const text=experimentText([...basket.values()],manifest.epoch);
      sessionStorage.setItem('solar-catalog-experiment',JSON.stringify({text,snapshot:manifest.sourceSha256,count:basket.size}));
      location.href=`${base}simulator/?experiment=1`;
    } catch(error) {basketStatus.textContent=errorMessage(error);}
  };
  download.onclick=()=>{
    try { downloadText(experimentText([...basket.values()],manifest.epoch),'solar-experiment.tsv','text/tab-separated-values'); }
    catch(error) {basketStatus.textContent=errorMessage(error);}
  };
  if (!globalThis.isSecureContext || !globalThis.crypto?.subtle) {
    // Plain-HTTP LAN previews lack Web Crypto; integrity checks stay mandatory.
    status.textContent=integrityUnavailableMessage;
    get('[data-map-status]').textContent='Map unavailable without verified downloads; source documentation remains available.';
    form.querySelectorAll<HTMLButtonElement|HTMLInputElement|HTMLSelectElement>('button,input,select').forEach(control=>{control.disabled=true;});
    return;
  }
  try {
    const response=await fetch(`${catalogBase}manifest.json`,{cache:'no-cache'});
    if(!response.ok) throw new Error('Catalog manifest unavailable.');
    manifest=await response.json(); if(manifest.schema!==2) throw new Error('Unsupported catalog schema.');
    const wasm=await fetch(`${base}wasm/catalog-orbits.wasm?revision=${encodeURIComponent(root.dataset.revision!)}`);
    if(!wasm.ok) throw new Error('C orbital preview module unavailable.');
    const compiled=await WebAssembly.instantiate(await wasm.arrayBuffer(),{});
    kernel=compiled.instance.exports as unknown as Kernel;kernel._initialize?.();
    density=await fetchPacked<Density>(catalogBase,manifest.overview,manifest.sourceSha256); draw();
    worker=new Worker(new URL('./catalog.worker.ts',import.meta.url),{type:'module'});
    const search=()=>{
      const data=new FormData(form);++request;
      previous.disabled=next.disabled=true;status.textContent='Searching catalog…';consent.hidden=true;
      get('[data-page]').textContent=`Page ${page+1}`;
      worker.postMessage({type:'search',base:catalogBase,manifest,query:String(data.get('query')??''),group:group.value,
        qmax:data.get('qmax')?Number(data.get('qmax')):Infinity,size:String(data.get('size')??''),page,request,allowFullScan:fullScanAccepted});
      draw();
    };
    const showResults=(message:SearchReply)=>{
      const hits=message.hits!;
      const resultKey = hits.map(hit => hit.id).join(',');
      if (resultKey !== shownResults) get('[data-results]').replaceChildren(...hits.map(hit=>{
        const tr=document.createElement('tr'),td=document.createElement('td'),button=document.createElement('button');
        button.textContent=hit.name;button.dataset.catalogId=String(hit.id);button.setAttribute('aria-haspopup','dialog');button.onclick=()=>void inspect(hit.id,hit.group);td.append(button);tr.append(td);
        for(const [label,text] of [['Class',manifest.classNames[hit.group]??hit.group],['Perihelion',hit.perihelion==null?'Unknown':`${hit.perihelion.toFixed(3)} AU`],['Orbit',hit.orbit?'Available':'Unavailable']]){const cell=document.createElement('td');cell.dataset.label=label;cell.textContent=text;tr.append(cell);}
        return tr;
      }));
      shownResults = resultKey;
      const total=message.total!;
      status.textContent=`${number(total)} ${total===1?'match':'matches'}; ${hits.length} ${hits.length===1?'row':'rows'} displayed. No catalog objects are simulated by this map.`;
      previous.disabled=page===0;next.disabled=(page+1)*50>=total;
    };
    const megabytes=(bytes:number)=>(bytes/1e6).toFixed(1);
    const showBuild=(message:BuildReply)=>{
      if(message.state==='progress'){
        if(scanProgress.hidden){scanProgress.hidden=false;announceProgress=createProgressAnnouncer();}
        scanBar.max=message.totalFiles;scanBar.value=message.files;
        scanText.textContent=`${message.files} of ${message.totalFiles} files (${megabytes(message.bytes)} of ${megabytes(message.totalBytes)} MB)`;
        if(announceProgress(message.files,message.totalFiles)) scanAnnounce.textContent=`Catalog index download ${Math.floor(message.files*100/message.totalFiles)}% complete.`;
        return;
      }
      scanProgress.hidden=true;
      scanAnnounce.textContent=message.state==='done'?'Catalog index ready; searches now run from memory.'
        :message.state==='cancelled'?'Catalog index download stopped.':`Catalog index download failed: ${message.error}`;
      if(message.state!=='done') fullScanAccepted=false;
    };
    worker.onmessage=event=>{
      const message=event.data as SearchReply|RecordReply|BuildReply;
      if(message.type==='build'){showBuild(message);return;}
      if(message.type==='record'){
        const pending=pendingRecords.get(message.request);pendingRecords.delete(message.request);
        if(message.record) pending?.resolve(message.record); else pending?.reject(new Error(message.error));
        return;
      }
      if(message.request!==request)return;
      if(!message.done){status.textContent='Waiting for the catalog index download to finish this search…';return;}
      if(message.error){status.textContent=message.error;return;}
      if(message.cancelled){status.textContent='Index download stopped. Catalog numbers, SPK IDs and unfiltered browsing still work; search again to restart the download.';return;}
      if(message.needsFullScan){
        consent.hidden=false;
        status.textContent=`This search needs the full catalog index (about ${(message.needsFullScan.bytes/1e6).toFixed(0)} MB). Choose Download index and search to continue.`;
        confirmScan.focus();
        return;
      }
      showResults(message);
    };
    worker.onerror=()=>{
      status.textContent='Catalog search worker failed. Reload to retry.';scanProgress.hidden=true;
      // No reply will arrive for outstanding lookups; settle them so the dialog shows the failure.
      pendingRecords.forEach(pending=>pending.reject(new Error('Catalog worker failed. Reload to retry.')));pendingRecords.clear();
    };
    confirmScan.onclick=()=>{fullScanAccepted=true;search();queryInput.focus();};
    dismissScan.onclick=()=>{consent.hidden=true;status.textContent='Search not run. Look up a catalog number or SPK ID, or browse an orbital class without other filters.';queryInput.focus();};
    // The worker confirms with a build 'cancelled' status, which hides the block.
    stopScan.onclick=()=>{fullScanAccepted=false;worker.postMessage({type:'cancel'});queryInput.focus();};
    form.onsubmit=event=>{event.preventDefault();page=0;search();};
    group.onchange=()=>{page=0;search();};
    previous.onclick=()=>{if(page){--page;search();}};next.onclick=()=>{++page;search();};
    search();
  } catch(error) {status.textContent=errorMessage(error);get('[data-map-status]').textContent='Map initialization failed; source documentation remains available.';}
}
