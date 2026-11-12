/* M17: a bounded host-only credits wager using the vanilla wallet transaction. */
#include "../../runtime/native/gameplay.h"
#include "support.h"
#include "../../runtime/native/input.h"
#include "shop_signal.h"
static PTR wallet,charge,decimal_ctor,decimal_long,realtime;
static long (__stdcall *random_bytes)(PTR,unsigned char *,DWORD,DWORD);
static int bet=5,win_percent=45,cooldown=4,disabled;
static float next_bet;
static DRoot world_root;
static int authority_set,authority,authority_lost,joined_initial;
static U64 host_initial,local_initial;
static const char *status="Moon Sweepers | Gambling | Ctrl+G wagers configured credits (default $5)";
static int wager_authority(void){
    int ok=1,joined=boolean(sx.joined,0,&ok);U64 host=0,local=0;
    if(joined){
        PTR h=call(sx.host,0,0,&ok);if(h)dcopy(&host,unbox(h),8);else ok=0;
        PTR l=call(sx.local,0,0,&ok);if(l)dcopy(&local,unbox(l),8);else ok=0;
    }
    if(!authority_set){authority_set=1;joined_initial=joined;host_initial=host;local_initial=local;authority=ok&&(!joined||(local&&host==local));}
    if(!ok||joined!=joined_initial||(joined&&(host!=host_initial||local!=local_initial)))authority_lost=1;
    return authority&&!authority_lost;
}
static void wager_world(PTR world){
    if(world==dget(world_root))return;
    ddrop(&world_root);next_bet=0;authority_set=authority=authority_lost=0;
    /* Only real world replacement resets admission; wallet uncertainty lasts
     * until process exit, including through menus and subsequent worlds. */
    if(world)world_root=dhold(world);
    if(!disabled)status="Moon Sweepers | Gambling | Ctrl+G wagers configured credits (default $5)";
}
static long long balance(DRoot world,DRoot player,int *ok){void *a[]={dget(player)};PTR money=call(wallet,dget(world),a,ok);if(!*ok||!money){*ok=0;return 0;}void *b[]={unbox(money)};PTR value=call(decimal_long,0,b,ok);if(!*ok||!value){*ok=0;return 0;}return *(long long *)unbox(value);}
/* Zero means preparation failed before ChargeBuild; negative means the native
 * mutation was attempted and its effect is uncertain. Never retry the latter. */
static int transact(DRoot world,DRoot player,int delta,long long expected){
    int ok=1;unsigned char amount[16]={0};void *a[]={&delta};call(decimal_ctor,amount,a,&ok);if(!ok)return 0;
    void *b[]={dget(player),amount};call(charge,dget(world),b,&ok);return ok&&balance(world,player,&ok)==expected&&ok?1:-1;
}
static void wager_run(DRoot world,DRoot player,float now){
    int ok=1;
    long long before=balance(world,player,&ok);if(!ok||before<0||before>1000000000){status="Moon Sweepers | Gambling | Wallet unavailable";return;}
    if(before<bet){status="Moon Sweepers | Gambling | Insufficient credits";sx_event("insufficient-funds",bet);return;}
    /* Rejection sampling keeps each outcome uniform; draw before charging. */
    unsigned char roll=255;int draws=0;while(roll>=200&&draws++<16)if(random_bytes(0,&roll,1,2)<0){status="Moon Sweepers | Gambling | Random source unavailable";sx_event("random-failed",0);return;}
    if(roll>=200){status="Moon Sweepers | Gambling | Random draw exhausted; no wager charged";sx_event("random-exhausted",0);return;}
    int won=(roll%100)<win_percent;
    int paid=transact(world,player,-bet,before-bet);
    if(!paid){status="Moon Sweepers | Gambling | Transaction preparation failed; no wager charged";sx_event("debit-prepare-failed",0);return;}
    next_bet=now+(float)cooldown;
    if(paid<0){disabled=1;status="Moon Sweepers | Gambling | Transaction failed; wagering disabled";sx_event("debit-failed",bet);return;}
    sx_event("wager-debited",bet);
    if(won){
        if(transact(world,player,bet*2,before+bet)!=1){disabled=1;status="Moon Sweepers | Gambling | Payout failed; wagering disabled";sx_event("payout-failed",bet*2);return;}
        status="Moon Sweepers | Gambling | WIN: double stake paid | Ctrl+G to play again";sx_event("win-net",bet);
    }else{status="Moon Sweepers | Gambling | Stake lost | Ctrl+G to play again";sx_event("loss-net",-bet);}
}
static void wager(DRoot world,float now){
    int ok=1;PTR player=call(sx.player,0,0,&ok);DRoot root=ok&&player?dhold(player):0;
    if(!root){status="Moon Sweepers | Gambling | Player unavailable";return;}
    wager_run(world,root,now);ddrop(&root);
}
static int update(int id,Context *c){
    (void)id;if(!sx_thread())return 0;PTR walker=(PTR)c->rcx,raw_world=0;field_value(walker,sx.world_field,&raw_world);wager_world(raw_world);
    /* Observe authority even while typing, unfocused or restoring. */
    int host=world_root?wager_authority():0;
    int ok=1;PTR world=sx_world(walker,&ok);int input_ready=world&&world_root&&ok&&sx_input(walker,&ok)&&ok;
    int allowed=input_ready&&host;
    MoonInputState input=moon_input_action("gambling.wager","Ctrl+G",allowed);int shop=moon_shop_poll();
    if(input_ready){
        if(!disabled&&!host)status="Moon Sweepers | Gambling | Host authority unavailable for this world";
        if(shop&&allowed&&!disabled)status="Moon Sweepers | Gambling counter | Ctrl+G confirms configured wager (default $5)";
        float now=dnumber(realtime,0,&ok);
        if(allowed&&!disabled&&input.pressed&&ok&&now>=0&&now<3.4e38f&&now>=next_bet)wager(world_root,now);
        sx_title(status);
    }
    return 0;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    (void)integer;
    if(!sx_start(domain,game,"gambling","MOONLAB_GAMBLING_CONFIG"))return 0;
    wallet=dmethod("MoonCleaners","MoonWorld","WalletDollars","System.String");charge=dmethod("MoonCleaners","MoonWorld","ChargeBuild","System.String;System.Decimal");
    decimal_ctor=dmethod("System","Decimal",".ctor","System.Int32");decimal_long=dmethod("System","Decimal","ToInt64","System.Decimal");realtime=dmethod("UnityEngine","Time","get_realtimeSinceStartup","");
    PTR bcrypt=LoadLibraryA("bcrypt.dll");random_bytes=bcrypt?(long(__stdcall *)(PTR,unsigned char *,DWORD,DWORD))GetProcAddress(bcrypt,"BCryptGenRandom"):0;
    if(!wallet||!charge||!decimal_ctor||!decimal_long||!realtime||!random_bytes||method_token(charge)!=100664941)return 0;
    bet=sx_limit("gambling","bet",5,1,100);win_percent=sx_limit("gambling","win_percent",45,1,50);cooldown=sx_limit("gambling","cooldown_seconds",4,2,120);
    moon_shop_listen("gambling");if(!sx_install(game,update)){moon_shop_unlisten();return 0;}sx_event("ready",bet);return 1;
}
