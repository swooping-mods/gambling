/* M17: a bounded host-only credits wager using the vanilla wallet transaction. */
#include "../../runtime/native/gameplay.h"
#include "support.h"
#include "../../runtime/native/input.h"
#include "shop_signal.h"
static PTR wallet,charge,decimal_ctor,decimal_long,realtime;
static long (__stdcall *random_bytes)(PTR,unsigned char *,DWORD,DWORD);
static int bet=5,win_percent=45,cooldown=4,disabled;
static float next_bet;
static const char *status="Moon Sweepers | Gambling | Ctrl+G wagers configured credits (default $5)";
static long long balance(PTR world,PTR player,int *ok){void *a[]={player};PTR money=call(wallet,world,a,ok);if(!money){*ok=0;return 0;}void *b[]={unbox(money)};PTR value=call(decimal_long,0,b,ok);if(!value){*ok=0;return 0;}return *(long long *)unbox(value);}
static int transact(PTR world,PTR player,int delta,long long expected){
    int ok=1;unsigned char amount[16]={0};void *a[]={&delta};call(decimal_ctor,amount,a,&ok);if(!ok)return 0;
    void *b[]={player,amount};call(charge,world,b,&ok);return ok&&balance(world,player,&ok)==expected&&ok;
}
static void wager(PTR world,float now){
    int ok=1;PTR player=call(sx.player,0,0,&ok);if(!player||!ok)return;
    long long before=balance(world,player,&ok);if(!ok||before<0||before>1000000000){status="Moon Sweepers | Gambling | Wallet unavailable";return;}
    if(before<bet){status="Moon Sweepers | Gambling | Insufficient credits";sx_event("insufficient-funds",bet);return;}
    /* Rejection sampling keeps each outcome uniform; draw before charging. */
    unsigned char roll=255;int draws=0;while(roll>=200&&draws++<16)if(random_bytes(0,&roll,1,2)<0){status="Moon Sweepers | Gambling | Random source unavailable";return;}
    if(roll>=200)return;int won=(roll%100)<win_percent;next_bet=now+(float)cooldown;
    if(!transact(world,player,-bet,before-bet)){disabled=1;status="Moon Sweepers | Gambling | Transaction failed; wagering disabled";sx_event("debit-failed",bet);return;}
    sx_event("wager-debited",bet);
    if(won){
        if(!transact(world,player,bet*2,before+bet)){disabled=1;status="Moon Sweepers | Gambling | Payout failed; wagering disabled";sx_event("payout-failed",bet*2);return;}
        status="Moon Sweepers | Gambling | WIN: double stake paid | Ctrl+G to play again";sx_event("win-net",bet);
    }else{status="Moon Sweepers | Gambling | Stake lost | Ctrl+G to play again";sx_event("loss-net",-bet);}
}
static int update(int id,Context *c){
    (void)id;if(!sx_thread())return 0;PTR walker=(PTR)c->rcx;int ok=1;PTR world=sx_world(walker,&ok);
    int allowed=world&&ok&&sx_input(walker,&ok)&&sx_host(&ok)&&ok;
    MoonInputState input=moon_input_action("gambling.wager","Ctrl+G",allowed);int shop=moon_shop_poll();
    if(!world||!ok){next_bet=0;return 0;}
    if(allowed){
        if(shop)status="Moon Sweepers | Gambling counter | Ctrl+G confirms configured wager (default $5)";
        float now=dnumber(realtime,0,&ok);
        if(!disabled&&input.pressed&&ok&&now>=next_bet)wager(world,now);
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
