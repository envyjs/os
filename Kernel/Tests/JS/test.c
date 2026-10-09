#include "../../Runtime/JS/js.h"
#include "../../Browser/browser.h"
static js_vm vm;
static char many_functions[8192];
static int host(js_vm *engine,int id,const js_value *args,uint32_t count,js_value *result) {
    (void)engine;
    if(id!=1||count!=1||args[0].type!=JS_NUMBER) return 0;
    *result=(js_value){JS_NUMBER,args[0].number*2,0,0}; return 1;
}
static uint32_t length(const char *s) { uint32_t n=0; while(s[n]) n++; return n; }
static int run(const char *s) { return js_eval(&vm,s,length(s)); }
static int same(const char *a,const char *b) { while(*a&&*a==*b) {a++;b++;} return *a==*b; }
int js_self_test(void) {
#define CHECK(condition,code) do { if(!(condition)) return code; } while(0)
    js_init(&vm,host,0);
    CHECK(run("var n = 1 + 2; n = n + 4;"),161);
    CHECK(js_get(&vm,0,"n").number==7,162);
    CHECK(run("var s='ab' + \"cd\";"),163);
    CHECK(same(js_get(&vm,0,"s").text,"abcd"),164);
    CHECK(run("var empty; empty = 12;"),165);
    CHECK(js_get(&vm,0,"empty").number==12,166);
    CHECK(js_set(&vm,0,"twice",js_native(1)),167);
    CHECK(run("var result=twice(n);"),168);
    CHECK(js_get(&vm,0,"result").number==14,169);
    js_value object=js_object(&vm); CHECK(js_set(&vm,0,"node",object),170);
    CHECK(run("node.textContent='hello';"),171);
    CHECK(same(js_get(&vm,object.number,"textContent").text,"hello"),172);
    CHECK(run("var callback=function(){ node.textContent='called'; };"),173);
    CHECK(js_call(&vm,js_get(&vm,0,"callback")),174);
    CHECK(same(js_get(&vm,object.number,"textContent").text,"called"),175);
    CHECK(run("// line comment\nlet x=3\n/* block */ x=x+2;"),176);
    CHECK(js_get(&vm,0,"x").number==5,177);
    CHECK(run("var text='escaped\\nline';"),178);
    CHECK(same(js_get(&vm,0,"text").text,"escaped\nline"),179);
    CHECK(!run("var x='unterminated"),180);
    CHECK(!run("var x=2147483648;"),181);
    CHECK(!run("var x=2147483647+1;"),182);
    CHECK(!run("unknownName();"),183);
    CHECK(run("function missing(){}"),184);
    CHECK(run("var arrayExample=[1,2];var arrayFirst=arrayExample[0];"),185);
    CHECK(!run("var x=1 var y=2;"),186);
    CHECK(!run("var x='\\u1234';"),187);
    CHECK(!run("var fn=function(){"),188);
    CHECK(run("const fixed=1;"),189);
    CHECK(run("var valid=9;"),190); /* Errors don't poison a later script. */
    CHECK(js_get(&vm,0,"valid").number==9,191);
    CHECK(run("var sum=add(3,4); function add(a,b){return a+b;}"),192);
    CHECK(js_get(&vm,0,"sum").number==7,193);
    CHECK(run("var identity=function privateName(a){return a;}; var answer=identity(42);"),194);
    CHECK(js_get(&vm,0,"answer").number==42,195);
    CHECK(js_get(&vm,0,"privateName").type==JS_UNDEFINED,196);
    CHECK(run("function outer(a){var local=2; return function(b){a=a+b; return a+local;};} var closure=outer(10); var first=closure(3); var second=closure(4);"),197);
    CHECK(js_get(&vm,0,"first").number==15&&js_get(&vm,0,"second").number==19,198);
    CHECK(js_get(&vm,0,"a").type==JS_UNDEFINED&&js_get(&vm,0,"local").type==JS_UNDEFINED,199);
    CHECK(run("function absent(a){return a;} var omitted=absent(); function bare(){return;} var nothing=bare();"),200);
    CHECK(js_get(&vm,0,"omitted").type==JS_UNDEFINED&&js_get(&vm,0,"nothing").type==JS_UNDEFINED,201);
    CHECK(run("function newline(){return\n42;} var automatic=newline();"),202);
    CHECK(js_get(&vm,0,"automatic").type==JS_UNDEFINED,203);
    CHECK(run("function stop(){return 5; unknownName();} var stopped=stop();"),204);
    CHECK(js_get(&vm,0,"stopped").number==5,205);
    CHECK(run("function duplicate(){return 1;} function duplicate(){return 2;} var last=duplicate();"),206);
    CHECK(js_get(&vm,0,"last").number==2,207);
    CHECK(!run("return 1;"),208);
    CHECK(!run("function bad(a=){}"),209);
    CHECK(!run("function recursive(){recursive();} recursive();"),210);
    CHECK(vm.depth==0&&vm.scope==0,211);
    CHECK(js_call(&vm,js_get(&vm,0,"callback")),212);
    CHECK(!js_call(0,js_get(&vm,0,"callback")),213);
    CHECK(!run("function(){}"),214);
    CHECK(run("var wrapped=(function hidden(){return 6;}); var wrappedResult=wrapped();"),215);
    CHECK(js_get(&vm,0,"wrappedResult").number==6&&js_get(&vm,0,"hidden").type==JS_UNDEFINED,216);
    CHECK(!browser_key(0xe0)&&!browser_key(0x4f),217); /* End alone. */
    CHECK(!browser_key(0xe0)&&!browser_key(0xcf),218);
    CHECK(!browser_key(0x1d)&&!browser_key(0xe0)&&browser_key(0x4f),219);
    CHECK(!browser_key(0xe0)&&!browser_key(0x4f),220); /* Ignore repeat. */
    CHECK(!browser_key(0xe0)&&!browser_key(0xcf)&&!browser_key(0x9d),221);
    CHECK(!browser_key(0xe0)&&!browser_key(0x1d)&&!browser_key(0xe0)&&browser_key(0x4f),222);
    CHECK(!browser_key(0xe0)&&!browser_key(0xcf)&&!browser_key(0xe0)&&!browser_key(0x9d),223);
    CHECK(!browser_key(0xe0)&&!browser_key(0x4f),224); /* Released Ctrl. */
    CHECK(run("var defaultsCalled=0; function nextDefault(){defaultsCalled=defaultsCalled+1;return 7;} function defaults(a=nextDefault(),b=a+2){return a+b;} var defaulted=defaults(); var provided=defaults(0,4); var explicit=defaults(undefined,1);"),225);
    CHECK(js_get(&vm,0,"defaulted").number==16&&js_get(&vm,0,"provided").number==4&&js_get(&vm,0,"explicit").number==8,226);
    CHECK(js_get(&vm,0,"defaultsCalled").number==2,227);
    CHECK(run("function makeDefault(seed){return function(a=seed){return a;};} var defaultClosure=makeDefault(12);var capturedDefault=defaultClosure();"),228);
    CHECK(js_get(&vm,0,"capturedDefault").number==12,229);
    CHECK(run("var label=`size ${defaulted}px\\n${provided}`;var css=`body {\n color: red;\n}`;"),230);
    CHECK(same(js_get(&vm,0,"label").text,"size 16px\n4"),231);
    CHECK(run("function conditional(a=0){if(a){return 9;}else{return 3;}}var yes=conditional(1);var no=conditional();if(false){unknownName();}"),232);
    CHECK(js_get(&vm,0,"yes").number==9&&js_get(&vm,0,"no").number==3,233);
    CHECK(!run("fixed=2;"),234);
    CHECK(!run("const missingInitializer;"),235);
    CHECK(!run("var badTemplate=`${1`;"),236);
    CHECK(run("function nullable(a=4){return a;}var keptNull=nullable(null);var keptEmpty=nullable('');var keptFalse=nullable(false);"),237);
    CHECK(js_get(&vm,0,"keptNull").type==JS_NULL&&js_get(&vm,0,"keptEmpty").length==0&&js_get(&vm,0,"keptFalse").number==0,238);
    CHECK(!run("var later=9;function premature(a=later,later=2){return a;}premature();"),239);
    CHECK(!run("function selfDefault(a=a){return a;}selfDefault();"),240);
    CHECK(run("var increment=a=>a+1;var arrowResult=increment(5);var blockArrow=(a=3)=>{return a+2;};var arrowDefault=blockArrow();var options={method:'HEAD',nested:{ok:1}};"),241);
    CHECK(js_get(&vm,0,"arrowResult").number==6&&js_get(&vm,0,"arrowDefault").number==5,242);
    CHECK(same(js_get(&vm,js_get(&vm,0,"options").number,"method").text,"HEAD"),243);
    CHECK(run("const prerelease='true';const type='desktop';const devmodeenabled='no';var selected=0;if(prerelease === 'true'){selected=selected+1;}if(devmodeenabled === 'no'){selected=selected+2;}if(type === 'desktop'){selected=selected+4;}if(type === 'go'){unknownName();}"),244);
    CHECK(js_get(&vm,0,"selected").number==7,245);
    CHECK(run("var equality=1+2===3;var mismatch=1==='1';var boolMismatch=true===1;var distinct=null!==undefined;var equalText='ab'==='a'+'b';var nested=0;if((type === 'desktop')){if(prerelease !== 'false'){nested=1;}}"),246);
    CHECK(js_get(&vm,0,"equality").number==1&&js_get(&vm,0,"mismatch").number==0&&js_get(&vm,0,"boolMismatch").number==0,247);
    CHECK(js_get(&vm,0,"distinct").number==1&&js_get(&vm,0,"equalText").number==1&&js_get(&vm,0,"nested").number==1,248);
    CHECK(!run("if(type ===){selected=99;}"),249);
    js_init(&vm,host,0);
    uint32_t used=0;
    for(uint32_t i=0;i<160;i++) {
        const char *prefix="function f",*suffix="(a=7){return a;}\n";
        while(*prefix) many_functions[used++]=*prefix++;
        many_functions[used++]=(char)('0'+i/100);
        many_functions[used++]=(char)('0'+i/10%10);
        many_functions[used++]=(char)('0'+i%10);
        while(*suffix) many_functions[used++]=*suffix++;
    }
    many_functions[used]=0;
    CHECK(run(many_functions)&&vm.function_count==160,250);
    CHECK(run(many_functions)&&vm.function_count==160,251); /* Hoisting reuses records. */
    CHECK(run("var oldResult=f000();var middleResult=f079();var newResult=f159(23);var after='still valid';"),252);
    CHECK(js_get(&vm,0,"oldResult").number==7&&js_get(&vm,0,"middleResult").number==7&&js_get(&vm,0,"newResult").number==23,253);
    CHECK(same(js_get(&vm,0,"after").text,"still valid"),254);
    vm.arena_end=vm.used;
    CHECK(!run("var exhausted=function(){};"),255);
    CHECK(!run("var exhausted='';"),256);
    js_init(&vm,host,0);
    CHECK(vm.functions==0&&vm.function_count==0&&vm.arena_end==sizeof(vm.strings),257);
    CHECK(run("function fresh(){return 8;}var freshResult=fresh();"),258);
    CHECK(js_get(&vm,0,"freshResult").number==8,259);
    CHECK(!run("\nfunction location(){\n if(true){\n unknownIdentifier();\n }\n}\nlocation();"),260);
    CHECK(vm.error_line==4,261);
    CHECK(run("var minute=7;var padded=minute.toString().padStart(2,'0');var repeated='x'.padStart(6,'ab');"),262);
    CHECK(same(js_get(&vm,0,"padded").text,"07")&&same(js_get(&vm,0,"repeated").text,"ababax"),263);
    CHECK(run("function labels(){const days=['Sunday','Monday'];return days[1];}var dayLabel=labels();"),264);
    CHECK(same(js_get(&vm,0,"dayLabel").text,"Monday"),265);
    CHECK(run("function transient(){var object={value:7};return object.value;}function escapeObject(){return {value:11};}var keptObject=escapeObject();"),266);
    uint32_t environments=vm.environment_count,objects=vm.objects,properties=vm.property_count;
    for(uint32_t i=0;i<500;i++){js_value result;CHECK(js_call_args(&vm,js_get(&vm,0,"transient"),0,0,&result)&&result.number==7,267);}
    CHECK(vm.environment_count==environments&&vm.objects==objects&&vm.property_count==properties,268);
    CHECK(js_get(&vm,js_get(&vm,0,"keptObject").number,"value").number==11,269);
    js_init(&vm,host,0);
    CHECK(run("const preserved=17;function retained(){return preserved;}"),270);
    for(uint32_t i=0;i<2048;i++) {
        char key[6]={'p',(char)('0'+i/1000),(char)('0'+i/100%10),(char)('0'+i/10%10),(char)('0'+i%10),0};
        CHECK(js_set(&vm,0,key,(js_value){JS_NUMBER,(int32_t)i,0,0}),271);
    }
    for(uint32_t i=0;i<2048;i++) {
        char key[6]={'p',(char)('0'+i/1000),(char)('0'+i/100%10),(char)('0'+i/10%10),(char)('0'+i%10),0};
        CHECK(js_get(&vm,0,key).number==(int32_t)i,272);
    }
    CHECK(run("var retainedResult=retained();p2047=99;"),273);
    CHECK(js_get(&vm,0,"retainedResult").number==17&&js_get(&vm,0,"p2047").number==99,274);
    CHECK(!run("preserved=18;"),275);
    vm.error=0;
    while(vm.property_count<vm.property_capacity) {
        char key[6]={'q',(char)('0'+vm.property_count/1000),(char)('0'+vm.property_count/100%10),(char)('0'+vm.property_count/10%10),(char)('0'+vm.property_count%10),0};
        CHECK(js_set(&vm,0,key,(js_value){JS_NUMBER,1,0,0}),279);
    }
    vm.used=vm.arena_end;
    CHECK(!js_set(&vm,0,"overflow",(js_value){JS_NUMBER,1,0,0}),276);
    CHECK(same(vm.error,"JavaScript memory exhausted"),277);
    js_init(&vm,host,0);
    CHECK(vm.properties==0&&vm.property_capacity==0&&vm.property_count==0,278);
    return 0;
}
