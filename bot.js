/* PAPERBOI autopilot — a synthetic *player*, not part of the game.
   It reads the same world state the renderer draws (via the wasm's read-only
   `st` view) and produces literal thumb inputs: [jx, jy, tap], exactly what a
   finger on the screen produces. The game cannot tell the difference.
   Used headless for accuracy testing and in-page as arcade attract mode. */
let bt=0;
function paperboiBot(V){
  bt++;
  if(V.mode[0]!==1)return[0,0,(bt&63)===0?1:0];          // menus: press start ~1/s
  const cam2=V.cam2[0],pv2=V.pv16[0]>>4,pu2=cam2+150,spd=V.spd16[0],papers=V.papers[0];
  /* steering: hold the mailbox lane, but only ever move through clear air */
  let want=6,dog=null;const blk=[];
  for(let i=0;i<28;i++){
    const t=V.ot[i];if(!t)continue;
    const u=V.ou[i]>>4,v=V.ov[i]>>4,du=u-pu2,dv=v-pv2;
    if(t===4){if(papers<=6&&du>0&&du<120)want=v}                // low on papers: grab the bundle
    else if(t===5){if(du>-30&&du<110&&Math.abs(dv)<40&&
      (!dog||Math.abs(du)<Math.abs(dog.du)))dog={du,dv,v};
      if(du>-12&&du<60)blk.push({v,du,r:12})}
    else if(du>-12&&du<80)blk.push({v,du,r:12});
  }
  const car=(V.cu[0]>>4)-pu2;                                   // oncoming car, lane ~64
  if(car>-24&&car<140)blk.push({v:56,du:car,r:22});
  let jy=-40;
  if(dog){ /* a dog closes 0.7px/f laterally vs our 2.4 and can't match our
              forward speed: hold a 26px gap to one side and sprint past */
    jy=-127;
    want=dog.dv>0?dog.v-26:dog.v+26;
    if(want<-40)want=dog.v+26;else if(want>30)want=dog.v-26;
  }
  const safe=v=>!blk.some(b=>Math.abs(v-b.v)<b.r);
  const crosses=(a,b)=>blk.some(x=>x.du>-12&&x.du<28&&        // about to be alongside:
    x.v>Math.min(a,b)-10&&x.v<Math.max(a,b)+10);              // never steer through its band
  want=Math.max(-40,Math.min(30,want));
  let target=want;
  if(!safe(want)||crosses(pv2,want)){                          // scan for best reachable lane
    target=null;let best=1e9;
    for(let v=-40;v<=30;v+=2){
      if(!safe(v)||crosses(pv2,v))continue;
      const cost=Math.abs(v-want)+Math.abs(v-pv2)/2;
      if(cost<best){best=cost;target=v}}
    if(target===null){target=pv2;jy=127}                       // boxed in: brake hard, hold lane
    else if(!dog)jy=-70;
  }
  const jx=Math.max(-127,Math.min(127,(target-pv2)*14));
  /* throwing: only when the exact forward-sim says this throw scores */
  const tap=papers>0&&!V.pon.some(p=>p)&&simThrow(pu2,V.pv16[0],spd,V)?1:0;
  return[jx,jy,tap];
}
/* frame-exact mirror of step()'s paper physics and target checks */
function simThrow(pu2,pv16,spd,V){
  let u16=(pu2+6)<<4,w16=pv16,z16=10<<4,vz=34;
  for(let t=0;t<48;t++){
    u16+=spd+8;w16-=54;z16+=vz;vz-=3;
    const u2=u16>>4,v2=w16>>4,z=z16>>4,um=((u2%200)+200)%200,
          hi=Math.floor(u2/200),ok=hi>=0&&hi<10;
    if(ok&&V.subs[hi]&&um>=124&&um<138&&v2>-22&&v2<-2&&z>=4&&z<=20)
      return V.deliv[hi]?0:1;                                   // mailbox eats the paper
    if(v2<=-62&&z<34&&ok&&um>=10&&um<130){const x=um-10;
      if(x>=88&&x<106&&z<26)return V.subs[hi]&&!V.deliv[hi]?1:0;// doorway
      if(x<84&&x%40>=10&&x%40<26&&z>=12&&z<=26&&!V.smash[hi])
        return!V.subs[hi]&&V.papers[0]>6?1:0;                   // window: only non-subs, only when flush
      return 0}                                                 // wall thud: wasted
    if(z<=0)return ok&&V.subs[hi]&&!V.deliv[hi]&&v2>=-62&&v2<=-46&&um>=94&&um<=120?1:0} // porch
  return 0;
}
if(typeof module!=="undefined")module.exports={paperboiBot,simThrow};
