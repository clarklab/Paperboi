/* PAPERBOI — a faithful Paperboy homage in one freestanding C file.
   clang --target=wasm32 -nostdlib. No engine, no libc, no assets:
   every pixel (world, sprites, font) is computed below.
   World space: u2 = distance along the street, v2 = lateral offset.
   Isometric 2:1 axes: +u -> screen(+1,-1/2), +v -> screen(+1,+1/2). */
typedef unsigned int u32; typedef unsigned short u16; typedef int i32;
enum{W=160,H=288,OX=-72,OY=269,PA2=150,NH=10,CELL=200,NOB=28,NP=4,NL=8};
#define EXP(n) __attribute__((export_name(n)))
#define RGB(r,g,b) (0xff000000u|((u32)(b)<<16)|((u32)(g)<<8)|(u32)(r))
static u32 fb[W*H];
static u32 rs=1; static u32 rnd(void){rs^=rs<<13;rs^=rs>>17;rs^=rs<<5;return rs;}
static i32 mod(i32 a,i32 m){i32 r=a%m;return r<0?r+m:r;}
static i32 clampi(i32 v,i32 a,i32 b){return v<a?a:v>b?b:v;}

/* ---- state ---- */
static i32 mode,tick,day,score,lives,papers,cam2,cam16,spd16,pv16,invuln,timer,banner;
static u32 ev;
static i32 subs[NH],deliv[NH],smash[NH];
static i32 ot[NOB],ou16[NOB],ov16[NOB],ost[NOB];      /* 1 hydrant 2 trash 3 grate 4 bundle 5 dog */
static i32 pon[NP],pu16[NP],pw16[NP],pz16[NP],pvz[NP]; /* papers in flight */
static i32 lu[NL],lv[NL],lt[NL];                       /* landed-paper litter */
static i32 cu16[2]; static const i32 CV[2]={64,116};   /* cars: near oncoming, far lane */
static const char*DAYN[7]={"MONDAY","TUESDAY","WEDNESDAY","THURSDAY","FRIDAY","SATURDAY","SUNDAY"};
static const u32 PAST[5]={RGB(150,220,180),RGB(235,170,190),RGB(240,225,170),RGB(160,200,235),RGB(200,180,230)};

/* ---- drawing ---- */
static void px(i32 x,i32 y,u32 c){if((u32)x<W&&(u32)y<H)fb[y*W+x]=c;}
static void vln(i32 x,i32 y,i32 h,u32 c){while(h-->0)px(x,y++,c);}
static void rect(i32 x,i32 y,i32 w,i32 h,u32 c){for(i32 i=0;i<w;i++)vln(x+i,y,h,c);}
static void disc(i32 x,i32 y,i32 r,u32 c){for(i32 j=-r;j<=r;j++)for(i32 i=-r;i<=r;i++)if(i*i+j*j<=r*r)px(x+i,y+j,c);}
static u32 dim(u32 c){return 0xff000000u|((c>>1)&0x7f7f7fu)|((c>>2)&0x3f3f3fu);}
static i32 isx(i32 u2,i32 v2){return OX+(u2-cam2)+v2;}
static i32 isy(i32 u2,i32 v2){return OY+((v2-(u2-cam2))>>1);}

/* 3x5 font, 0-9 A-Z, three bits per row */
#define G(a,b,c,d,e) ((a<<12)|(b<<9)|(c<<6)|(d<<3)|e)
static const u16 FONT[36]={
 G(7,5,5,5,7),G(2,6,2,2,7),G(7,1,7,4,7),G(7,1,3,1,7),G(5,5,7,1,1),G(7,4,7,1,7),G(7,4,7,5,7),G(7,1,2,2,2),G(7,5,7,5,7),G(7,5,7,1,7),
 G(7,5,7,5,5),G(6,5,6,5,6),G(7,4,4,4,7),G(6,5,5,5,6),G(7,4,7,4,7),G(7,4,7,4,4),G(7,4,5,5,7),G(5,5,7,5,5),G(7,2,2,2,7),G(1,1,1,5,7),
 G(5,5,6,5,5),G(4,4,4,4,7),G(5,7,7,5,5),G(5,7,7,7,5),G(7,5,5,5,7),G(7,5,7,4,4),G(7,5,5,7,3),G(7,5,6,5,5),G(7,4,7,1,7),G(7,2,2,2,2),
 G(5,5,5,5,7),G(5,5,5,5,2),G(5,5,7,7,5),G(5,5,2,5,5),G(5,5,2,2,2),G(7,1,2,4,7)};
static void txt(i32 x,i32 y,const char*s,u32 c,i32 sc){
 for(;*s;s++){
  if(*s!=' '){u16 b=FONT[*s<='9'?*s-'0':*s-'A'+10];
   for(i32 j=0;j<5;j++)for(i32 i=0;i<3;i++)if(b>>(14-j*3-i)&1)rect(x+i*sc,y+j*sc,sc,sc,c);}
  x+=4*sc;}}
static i32 tlen(const char*s){i32 n=0;while(s[n])n++;return n;}
static void ctxt(i32 y,const char*s,u32 c,i32 sc){txt((W-tlen(s)*4*sc+sc)/2,y,s,c,sc);}
static void num(i32 x,i32 y,i32 n,i32 dg,u32 c,i32 sc){
 for(i32 i=dg-1;i>=0;i--){char b[2]={(char)('0'+n%10),0};txt(x+i*4*sc,y,b,c,sc);n/=10;}}
static void scrim(void){for(i32 i=0;i<W*H;i++)fb[i]=dim(fb[i]);}

/* ---- ground: pure function of world position ---- */
static u32 ground(i32 u2,i32 v2){
 i32 um=mod(u2,CELL),hi=u2/CELL;
 if(v2<-70)return ((u2^v2)&7)?RGB(70,150,80):RGB(60,136,72);
 if(v2<-8){
  if(um>=140&&um<176)return ((u2^v2)&15)?RGB(160,158,150):RGB(140,138,132); /* driveway */
  if((u32)hi<NH&&subs[hi]&&um>=96&&um<118&&v2>=-62&&v2<-50)                 /* doormat */
   return (um==96||um==117||v2==-62||v2==-51)?RGB(120,70,40):RGB(190,150,90);
  u32 h=(u32)(u2*374761393u+v2*668265263u);h^=h>>13;
  if(v2<-16&&h%97==0)return (h&2)?RGB(240,120,160):RGB(250,220,90);        /* flowers */
  return ((u2^v2)&7)?RGB(88,180,96):RGB(74,160,84);}                       /* lawn */
 if(v2<24)return mod(u2,64)<3?RGB(150,150,160):((u2^v2)&15)?RGB(202,200,205):RGB(188,186,194);
 if(v2<30)return v2<26?RGB(230,228,232):RGB(120,122,134);                  /* curb */
 if(v2<142){if(v2>=84&&v2<88&&mod(u2,64)<30)return RGB(235,200,60);        /* lane dashes */
  return (((u2*5)^v2)&31)?RGB(96,100,110):RGB(120,124,132);}               /* asphalt */
 if(v2<148)return v2<144?RGB(225,224,230):RGB(118,120,130);
 if(v2<178)return ((u2^v2)&15)?RGB(198,196,202):RGB(182,180,188);
 return ((u2^v2)&7)?RGB(78,164,88):RGB(64,148,76);}

/* ---- procedural iso house: side wall + front wall + flat roof ---- */
static void house(i32 i){
 i32 a0=i*CELL+10-cam2; if(a0>480||a0<-240)return;
 u32 wall=subs[i]?PAST[i%5]:RGB(112,110,116),roof=subs[i]?RGB(150,72,60):RGB(76,74,80),side=dim(wall);
 for(i32 s=0;s<86;s++){i32 v2=-150+s;vln(OX+a0+v2,OY+((v2-a0)>>1)-33,34,side);}
 for(i32 t=0;t<120;t++){i32 a=a0+t,xx=OX+a-64,yt=OY+((-64-a)>>1)-33;
  for(i32 j=0;j<34;j++){u32 c=wall;i32 wx=mod(t,40);
   if(t>=88&&t<106&&j>=12){c=RGB(120,72,44);if(j>=20&&j<24&&t>=100&&t<103)c=RGB(230,200,90);}
   else if(t<84&&wx>=10&&wx<26&&j>=8&&j<20)
    c=smash[i]?RGB(18,18,26):((j^(wx>>2))&6)?RGB(150,210,235):RGB(196,236,250);
   else if(j==7||j==21)c=side;
   px(xx,yt+j,c);}}
 for(i32 t=0;t<124;t++)for(i32 s=0;s<88;s++){i32 a=a0+t-2,v2=-152+s;
  u32 c=((t^s)&7)?roof:dim(roof);if(s<3||t<3||s>84||t>120)c=dim(roof);
  px(OX+a+v2,OY+((v2-a)>>1)-34,c);}}
static void tree(i32 u2){
 i32 x=isx(u2,-84),y=isy(u2,-84);if(x<-12||x>W+12)return;
 rect(x-1,y-7,3,8,RGB(110,74,40));disc(x,y-11,7,RGB(48,124,58));disc(x-2,y-13,4,RGB(70,160,80));}

/* ---- sprites (depth sorted by screen y) ---- */
static void shadow(i32 x,i32 y,i32 w){rect(x-w/2,y,w,2,RGB(52,74,58));}
static void drhyd(i32 x,i32 y){shadow(x,y,7);rect(x-2,y-6,5,6,RGB(212,62,50));rect(x-1,y-8,3,2,RGB(230,120,90));px(x-3,y-4,RGB(230,120,90));px(x+3,y-4,RGB(230,120,90));}
static void drtrash(i32 x,i32 y){shadow(x,y,9);rect(x-3,y-8,7,8,RGB(178,182,190));for(i32 i=-3;i<4;i+=2)vln(x+i,y-8,8,RGB(150,154,164));rect(x-4,y-9,9,2,RGB(140,144,154));}
static void drgrate(i32 x,i32 y){rect(x-7,y-4,15,6,RGB(48,50,58));for(i32 i=-6;i<8;i+=3)vln(x+i,y-3,4,RGB(80,84,94));}
static void drbundle(i32 x,i32 y){shadow(x,y,9);rect(x-4,y-5,8,5,RGB(235,235,224));rect(x-4,y-3,8,1,RGB(180,178,168));vln(x,y-5,5,RGB(160,120,70));}
static void drdog(i32 x,i32 y,i32 st){
 shadow(x,y,9);i32 w=(tick>>2)&1;u32 c=RGB(152,102,60);
 rect(x-4,y-5,8,4,c);disc(x-5,y-6,2,c);px(x-6,y-8,c);px(x-4,y-8,c);      /* head+ears */
 px(x-7,y-6,RGB(30,30,30));px(x+4,y-6+w,c);                              /* eye, tail */
 px(x-3,y-1+w,dim(c));px(x+2,y-1+(1-w),dim(c));
 if(st&&(tick&8))txt(x-6,y-14,"WOOF",RGB(255,255,255),1);}
static void drmail(i32 i,i32 x,i32 y){
 shadow(x,y,5);vln(x,y-9,9,RGB(120,90,60));rect(x-3,y-13,7,5,deliv[i]?RGB(80,200,90):RGB(210,50,44));
 rect(x-3,y-13,7,1,RGB(240,240,240));if(!deliv[i])vln(x+4,y-16,4,RGB(255,90,60));}
static void drcar(i32 k){
 i32 cu=cu16[k]>>4,cv=CV[k],a0=cu-cam2;if(a0<-60||a0>460)return;
 u32 body=k?RGB(80,120,210):RGB(212,72,58);
 for(i32 s=0;s<16;s++){i32 v=cv-15+s;vln(OX+a0+v,OY+((v-a0)>>1)-8,7,dim(body));}     /* tail face */
 for(i32 t=0;t<34;t++){i32 a=a0+t;vln(OX+a+cv,OY+((cv-a)>>1)-8,7,body);}             /* near face */
 for(i32 t=0;t<34;t++)for(i32 s=0;s<16;s++){i32 a=a0+t,v=cv-15+s;
  u32 c=body;if(t>9&&t<26&&s>3&&s<13)c=RGB(150,210,240);if(t<2||t>31)c=dim(body);
  px(OX+a+v,OY+((v-a)>>1)-8,c);}
 rect(OX+a0+6+cv,OY+((cv-a0-6)>>1)-2,3,3,RGB(24,24,28));rect(OX+a0+27+cv,OY+((cv-a0-27)>>1)-2,3,3,RGB(24,24,28));
 px(OX+a0+cv-14,OY+((cv-14-a0)>>1)-5,RGB(255,240,150));}
static void drpaper(i32 x,i32 y,i32 z){
 shadow(x,y,4);if(tick&2)rect(x-1,y-z-1,3,2,RGB(245,245,235));else rect(x,y-z-1,2,3,RGB(245,245,235));}
static void drplayer(void){
 i32 pv2=pv16>>4,x=OX+PA2+pv2,y=OY+((pv2-PA2)>>1);
 if(invuln&&(tick&4))return;
 i32 ph=(tick>>3)&1;shadow(x,y,13);
 disc(x-4,y-2,3,RGB(28,28,34));disc(x+5,y-4,3,RGB(28,28,34));px(x-4,y-2,RGB(210,210,220));px(x+5,y-4,RGB(210,210,220));
 rect(x-2,y-5,5,1,RGB(224,54,40));px(x+4,y-6,RGB(224,54,40));px(x-3,y-4,RGB(224,54,40));
 px(x-1,y-4+ph,RGB(40,40,46));px(x+1,y-3-ph,RGB(40,40,46));                 /* pedals */
 rect(x-5,y-8,3,4,RGB(235,235,220));rect(x-5,y-8,3,1,RGB(200,60,50));       /* paper bag */
 rect(x-2,y-10,4,5,RGB(66,124,224));rect(x+2,y-9+ph,2,2,RGB(66,124,224));   /* torso+arm */
 disc(x,y-12,2,RGB(242,192,150));rect(x-2,y-15,5,2,RGB(235,235,235));rect(x+1,y-13,3,1,RGB(200,44,40));}

typedef struct{i32 sy,kind,idx;}Dr;
static Dr dl[48]; static i32 ndl;
static void put(i32 sy,i32 kind,i32 idx){if(ndl<48){dl[ndl].sy=sy;dl[ndl].kind=kind;dl[ndl].idx=idx;ndl++;}}

/* ---- game setup ---- */
static void genday(void){
 rs=(u32)(0x9b0+day*7777+1);for(i32 i=0;i<NOB;i++)ot[i]=0;i32 n=0;
 for(i32 i=0;i<NH&&n<NOB-4;i++){i32 b=i*CELL;
  if(i&&rnd()%3){ot[n]=1;ou16[n]=(b+30+(i32)(rnd()%90))<<4;ov16[n]=17<<4;n++;}
  if(rnd()&1){ot[n]=2;ou16[n]=(b+20+(i32)(rnd()%150))<<4;ov16[n]=21<<4;n++;}
  if(rnd()%3==0){ot[n]=3;ou16[n]=(b+20+(i32)(rnd()%150))<<4;ov16[n]=(34+(i32)(rnd()%70))<<4;n++;}
  if(i%4==2){ot[n]=4;ou16[n]=(b+120)<<4;ov16[n]=8<<4;n++;}
  if(i%3==1&&(i32)(rnd()%3)<=day/2){ot[n]=5;ou16[n]=(b+60)<<4;ov16[n]=-30*16;ost[n]=0;n++;}}
 for(i32 i=0;i<NH;i++)deliv[i]=smash[i]=0;
 for(i32 i=0;i<NP;i++)pon[i]=0;
 for(i32 i=0;i<NL;i++)lt[i]=0;
 cam16=0;cam2=0;papers=10;pv16=8<<4;spd16=22;invuln=0;banner=90;
 cu16[0]=(900+(i32)(rnd()%300))<<4;cu16[1]=-200*16;}
static void newgame(void){
 day=0;score=0;lives=3;rs=12345;
 for(i32 i=0;i<NH;i++)subs[i]=0;
 for(i32 n=0;n<6;){i32 i=(i32)(rnd()%NH);if(!subs[i]){subs[i]=1;n++;}}
 genday();}
static i32 nsubs(void){i32 n=0;for(i32 i=0;i<NH;i++)n+=subs[i];return n;}
static void crash(void){
 if(invuln)return;lives--;ev|=4;invuln=110;spd16=16;pv16=8<<4;
 if(lives<0){mode=3;timer=30;}}

/* ---- per-frame render ---- */
static void render(void){
 for(i32 y=0;y<H;y++){i32 dy=y-OY;
  for(i32 x=0;x<W;x++){i32 q=(x-OX)>>1;fb[y*W+x]=ground(q-dy+cam2,q+dy);}}
 for(i32 i=NH-1;i>=0;i--)house(i);
 for(i32 i=NH;i>=0;i--)tree(i*CELL-12);
 ndl=0;
 for(i32 i=0;i<NH;i++)if(subs[i]){i32 mu=i*CELL+131;if(mu-cam2>-20&&mu-cam2<450)put(isy(mu,-12),1,i);}
 for(i32 i=0;i<NOB;i++)if(ot[i]){i32 u=ou16[i]>>4;if(u-cam2>-30&&u-cam2<450)put(isy(u,ov16[i]>>4),2,i);}
 for(i32 k=0;k<2;k++)put(isy(cu16[k]>>4,CV[k]),3,k);
 for(i32 i=0;i<NP;i++)if(pon[i])put(isy(pu16[i]>>4,pw16[i]>>4),4,i);
 for(i32 i=0;i<NL;i++)if(lt[i])put(isy(lu[i],lv[i]),5,i);
 if(mode<2)put(isy(cam2+PA2,pv16>>4),0,0);
 for(i32 i=1;i<ndl;i++){Dr d=dl[i];i32 j=i;while(j&&dl[j-1].sy>d.sy){dl[j]=dl[j-1];j--;}dl[j]=d;}
 for(i32 i=0;i<ndl;i++){Dr d=dl[i];
  switch(d.kind){
   case 0:drplayer();break;
   case 1:drmail(d.idx,isx(d.idx*CELL+131,-12),d.sy);break;
   case 2:{i32 x=isx(ou16[d.idx]>>4,ov16[d.idx]>>4),t=ot[d.idx];
    if(t==1)drhyd(x,d.sy);else if(t==2)drtrash(x,d.sy);else if(t==3)drgrate(x,d.sy);
    else if(t==4)drbundle(x,d.sy);else drdog(x,d.sy,ost[d.idx]);break;}
   case 3:drcar(d.idx);break;
   case 4:drpaper(isx(pu16[d.idx]>>4,pw16[d.idx]>>4),d.sy,pz16[d.idx]>>4);break;
   case 5:rect(isx(lu[d.idx],lv[d.idx])-1,d.sy-1,3,2,RGB(240,240,230));break;}}
 /* HUD */
 rect(0,0,W,11,RGB(24,26,34));num(2,3,score,6,RGB(255,255,255),1);
 txt(W-2-tlen(DAYN[day])*4,3,DAYN[day],RGB(255,220,90),1);
 for(i32 i=0;i<papers;i++)rect(2+i*5,14,4,3,RGB(245,245,235));
 for(i32 i=0;i<lives;i++){rect(W-8-i*8,14,5,2,RGB(235,235,235));rect(W-7-i*8,16,3,1,RGB(200,44,40));}
 if(mode==1&&banner)ctxt(60,DAYN[day],RGB(255,255,255),2);
 if(mode==0){scrim();
  ctxt(70,"PAPERBOI",RGB(255,255,255),4);ctxt(104,"AN ISOMETRIC PAPER ROUTE",RGB(255,220,90),1);
  if(tick&32)ctxt(170,"TAP TO RIDE",RGB(255,255,255),2);
  ctxt(226,"HOLD AND DRAG TO STEER",RGB(200,200,210),1);
  ctxt(236,"TAP THROWS  2ND FINGER TOO",RGB(200,200,210),1);
  ctxt(246,"HIT MAILBOXES AND PORCHES",RGB(200,200,210),1);}
 if(mode==2){scrim();i32 d=0,s=0;
  for(i32 i=0;i<NH;i++){if(subs[i]&&deliv[i]&&!smash[i])d++;if(subs[i])s++;}
  ctxt(90,DAYN[day],RGB(255,255,255),2);ctxt(112,"COMPLETE",RGB(255,220,90),2);
  txt(44,140,"DELIVERED",RGB(255,255,255),1);num(84,140,d,1,RGB(120,255,140),1);
  txt(92,140,"OF",RGB(255,255,255),1);num(102,140,s,1,RGB(120,255,140),1);
  if(d==s)ctxt(156,"PERFECT  BONUS 500",RGB(120,255,140),1);
  else ctxt(156,"MISSED HOMES CANCEL",RGB(255,120,120),1);}
 if(mode==3){scrim();ctxt(100,"GAME OVER",RGB(255,90,80),3);
  ctxt(140,nsubs()?"TOO MANY CRASHES":"ALL SUBSCRIBERS LOST",RGB(255,255,255),1);
  txt(52,160,"SCORE",RGB(255,220,90),1);num(76,160,score,6,RGB(255,255,255),1);
  if(tick&32)ctxt(200,"TAP",RGB(255,255,255),2);}
 if(mode==4){scrim();ctxt(90,"SUNDAY DONE",RGB(120,255,140),2);
  ctxt(120,"PAPERBOI LEGEND",RGB(255,220,90),2);
  txt(52,160,"SCORE",RGB(255,220,90),1);num(76,160,score,6,RGB(255,255,255),1);
  if(tick&32)ctxt(200,"TAP",RGB(255,255,255),2);}}

/* ---- simulation ---- */
static void litter(i32 u2,i32 v2){for(i32 i=0;i<NL;i++)if(!lt[i]){lt[i]=240;lu[i]=u2;lv[i]=v2;return;}}
static void step(i32 jx,i32 jy,i32 taps){
 i32 pu2=cam2+PA2,pv2=pv16>>4;
 spd16=clampi(22-(jy>>3)+day,10,44);cam16+=spd16;cam2=cam16>>4;
 pv16=clampi(pv16+((jx*5)>>4),-64*16,140*16);pv2=pv16>>4;
 if(banner)banner--; if(invuln)invuln--;
 if(taps&&papers>0&&!pon[0]){/* throw left, slight forward carry */
  for(i32 i=0;i<NP;i++)if(!pon[i]){pon[i]=1;pu16[i]=(pu2+6)<<4;pw16[i]=pv16;pz16[i]=10<<4;pvz[i]=34;papers--;ev|=1;break;}}
 for(i32 i=0;i<NP;i++)if(pon[i]){
  pu16[i]+=spd16+8;pw16[i]-=54;pz16[i]+=pvz[i];pvz[i]-=3;
  i32 u2=pu16[i]>>4,v2=pw16[i]>>4,z=pz16[i]>>4,um=mod(u2,CELL),hi=u2/CELL,ok=(u32)hi<NH;
  if(ok&&subs[hi]&&um>=124&&um<138&&v2>-22&&v2<-2&&z>=4&&z<=20){ /* thwack: mailbox */
   if(!deliv[hi]){deliv[hi]=1;score+=250;}ev|=2;pon[i]=0;continue;}
  if(v2<=-62&&z<34){ /* the house wall plane */
   if(ok&&um>=10&&um<130){i32 t=um-10;
    if(t>=88&&t<106&&z<26){if(!deliv[hi]&&subs[hi]){deliv[hi]=1;score+=100;}ev|=2;}
    else if(t<84&&mod(t,40)>=10&&mod(t,40)<26&&z>=12&&z<=26&&!smash[hi]){
     smash[hi]=1;ev|=8;if(!subs[hi])score+=50;}
    pon[i]=0;continue;}}
  if(z<=0){if(ok&&subs[hi]&&!deliv[hi]&&v2>=-62&&v2<=-46&&um>=94&&um<=120){deliv[hi]=1;score+=100;ev|=2;}
   else litter(u2,v2);
   pon[i]=0;}}
 for(i32 i=0;i<NL;i++)if(lt[i])lt[i]--;
 /* cars */
 cu16[0]-=40+(day<<2);if((cu16[0]>>4)<cam2-120)cu16[0]=(cam2+520+(i32)(rnd()%400))<<4;
 cu16[1]+=spd16+12;if((cu16[1]>>4)>cam2+520)cu16[1]=(cam2-180)*16;
 for(i32 k=0;k<2;k++){i32 du=(cu16[k]>>4)+17-pu2,dv=CV[k]-8-pv2;
  if(!invuln&&du>-20&&du<20&&dv>-11&&dv<11)crash();}
 /* obstacles */
 for(i32 i=0;i<NOB;i++)if(ot[i]){
  i32 u=ou16[i]>>4,v=ov16[i]>>4,du=u-pu2,dv=v-pv2;
  if(ot[i]==5){ /* dog */
   if(!ost[i]&&du<90&&du>-10&&day+1){ost[i]=1;ev|=64;}
   if(ost[i]==1){if(du<-70)ost[i]=2;
    else{ou16[i]+=clampi(((pu2-4-u)<<4)/8,-26,26)/2;ov16[i]+=clampi(((pv2-v)<<4)/8,-22,22)/2;}}
   if(!invuln&&du>-8&&du<8&&dv>-7&&dv<7)crash();}
  else if(ot[i]==4){if(du>-10&&du<10&&dv>-9&&dv<9){papers=clampi(papers+5,0,10);ot[i]=0;ev|=16;}}
  else if(!invuln&&du>-9&&du<9&&dv>-7&&dv<7)crash();}
 if(!invuln&&pv2<=-58)crash();     /* hedges */
 if(!invuln&&pv2>=138)crash();     /* far curb */
 if(cam2>=NH*CELL+140){mode=2;timer=190;ev|=32;}}

static void nextday(void){
 i32 perfect=1;
 for(i32 i=0;i<NH;i++)if(subs[i]&&(!deliv[i]||smash[i]))perfect=0;
 for(i32 i=0;i<NH;i++)if(subs[i]&&(!deliv[i]||smash[i]))subs[i]=0;
 if(perfect){score+=500;for(i32 i=0;i<NH;i++)if(!subs[i]){subs[i]=1;break;}}
 if(!nsubs()){mode=3;timer=30;return;}
 day++;if(day>6){mode=4;timer=30;return;}
 genday();mode=1;}

/* ---- exports ---- */
EXP("fbp") i32 fbp(void){return (i32)fb;}
EXP("boot") void boot(u32 seed){rs=seed|1;newgame();mode=0;tick=0;}
EXP("frame") u32 frame(i32 jx,i32 jy,i32 taps){
 ev=0;tick++;
 if(mode==0){cam16+=20;cam2=cam16>>4;pv16=8<<4;
  if(cam2>=NH*CELL)cam16=cam2=0;
  if(taps){newgame();mode=1;}}
 else if(mode==1)step(jx,jy,taps);
 else if(mode==2){if(timer)timer--;if(!timer||taps)nextday();}
 else if(timer)timer--;
 else if(taps){newgame();mode=0;cam16=cam2=0;}
 render();return ev;}
