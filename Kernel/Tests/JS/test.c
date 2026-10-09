#include "../../Runtime/JS/js.h"
#include "../../Browser/browser.h"
static js_vm vm;
static char many_functions[8192];
static char many_statements[120000];
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
    CHECK(!run("var x=9223372036854775808;"),181);
    CHECK(!run("var x=9223372036854775807+1;"),182);
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
    CHECK(run("let hasClicked=false;var clicks=0;var desktopClick=(e)=>{if(!hasClicked){hasClicked=true;clicks=clicks+1;}};"),280);
    CHECK(js_call(&vm,js_get(&vm,0,"desktopClick"))&&js_call(&vm,js_get(&vm,0,"desktopClick")),281);
    CHECK(js_get(&vm,0,"clicks").number==1&&js_get(&vm,0,"hasClicked").number==1,282);
    CHECK(run("var notFalse=!false;var notTrue=!true;var notZero=!0;var notEmpty=!'';var notNull=!null;var doubleNot=!!17;var notCall=!function(){return false;}();var neg=-7;var signed=2 + -3;"),283);
    CHECK(js_get(&vm,0,"notFalse").number==1&&js_get(&vm,0,"notTrue").number==0&&js_get(&vm,0,"notZero").number==1&&js_get(&vm,0,"notEmpty").number==1&&js_get(&vm,0,"notNull").number==1&&js_get(&vm,0,"doubleNot").number==1&&js_get(&vm,0,"notCall").number==1&&js_get(&vm,0,"neg").number==-7&&js_get(&vm,0,"signed").number==-1,284);
    js_init(&vm,host,0);js_value parsed;
    const char *json="[{\"name\":\"Calculator\",\"url\":\"./Apps/CalculatorLeg.js\",\"enabled\":true},null,-7,\"\\u00e9\\ud83d\\ude00\"]";
    CHECK(js_json_parse(&vm,json,length(json),&parsed),285);
    CHECK(js_get(&vm,parsed.number,"length").number==4&&js_get(&vm,parsed.number,"1").type==JS_NULL&&js_get(&vm,parsed.number,"2").number==-7&&js_get(&vm,parsed.number,"3").length==6,286);
    CHECK(js_set(&vm,0,"data",parsed)&&run("var names='';data.forEach((item,index)=>{if(index===0){names=item.name;}});"),287);
    CHECK(same(js_get(&vm,0,"names").text,"Calculator"),288);
    CHECK(!js_json_parse(&vm,"[1,]",4,&parsed),289);vm.error=0;
    CHECK(!js_json_parse(&vm,"{\"a\":1} garbage",15,&parsed),290);vm.error=0;
    CHECK(!js_json_parse(&vm,"01",2,&parsed),291);vm.error=0;
    CHECK(!js_json_parse(&vm,"1.5",3,&parsed),292);
    js_init(&vm,host,0);
    /* Cross the former object cap and the old lexical-scope namespace. */
    js_value high;
    for(uint32_t i=0;i<65537;i++){high=js_object(&vm);CHECK(high.type==JS_OBJECT,293);}
    CHECK(high.number==65537&&js_set(&vm,high.number,"local",(js_value){JS_NUMBER,77,0,0}),294);
    CHECK(run("function scoped(){var local=9;return local;}var scopedResult=scoped();var later={value:23};"),295);
    CHECK(js_get(&vm,0,"scopedResult").number==9&&js_get(&vm,high.number,"local").number==77,296);
    CHECK(js_get(&vm,js_get(&vm,0,"later").number,"value").number==23,297);
    CHECK(run("function factory(){var items=[3,4];return items.forEach.bind(items);}var each=factory();var total=0;each(item=>{total=total+item;});"),298);
    CHECK(js_get(&vm,0,"total").number==7,299);
    vm.used=vm.arena_end;
    CHECK(js_object(&vm).type==JS_UNDEFINED&&same(vm.error,"JavaScript memory exhausted"),300);
    js_init(&vm,host,0);
    CHECK(run("var stamp=1791580000123;var stampText=`window-${stamp}`;var stampString=stamp.toString();var wideSum=stamp+7;function callable(){return 9;}callable.tag='function property';var tag=callable.tag;var called=callable();"),307);
    CHECK(same(js_get(&vm,0,"stampText").text,"window-1791580000123")&&same(js_get(&vm,0,"stampString").text,"1791580000123")&&js_get(&vm,0,"wideSum").number==1791580000130ll&&js_get(&vm,0,"called").number==9,308);
    CHECK(same(js_get(&vm,0,"tag").text,"function property"),309);
    CHECK(js_set(&vm,0,"native",js_native(1))&&js_set(&vm,js_property_owner(js_native(1)),"method",js_native(1))&&run("var nativeResult=native.method(7);var indexedResult=native['method'](8);"),310);
    CHECK(js_get(&vm,0,"nativeResult").number==14&&js_get(&vm,0,"indexedResult").number==16,311);
    CHECK(run("var sideEffects=0;function effect(){sideEffects++;return 'effect';}var chosen='chosen'||effect();var blocked=false&&effect();var fallback=''||effect();var precedence=true||false&&effect();var nested=true||(missing({x:[1,2]},undefined.property));var index=3;var previous=index++;var after=index--;"),312);
    CHECK(js_get(&vm,0,"sideEffects").number==1&&same(js_get(&vm,0,"chosen").text,"chosen")&&js_get(&vm,0,"blocked").type==JS_BOOLEAN&&js_get(&vm,0,"previous").number==3&&js_get(&vm,0,"after").number==4&&js_get(&vm,0,"index").number==3,313);
    CHECK(run("const fixed=7;"),314);CHECK(!run("fixed++;"),315);vm.error=0;
    CHECK(!run("var big=9223372036854775807;big++;"),316);
    js_init(&vm,host,0);CHECK(js_set(&vm,0,"minimum",(js_value){JS_NUMBER,INT64_MIN,0,0})&&run("var minimumText=minimum.toString();"),317);
    CHECK(same(js_get(&vm,0,"minimumText").text,"-9223372036854775808"),318);
    js_init(&vm,host,0);
    CHECK(run("function label(value='default'){return value;}var defaultLabel=label();var quoted=()=> 'quoted';var templated=()=> `template`;var quotedLabel=quoted();var templateLabel=templated();"),319);
    CHECK(same(js_get(&vm,0,"defaultLabel").text,"default")&&same(js_get(&vm,0,"quotedLabel").text,"quoted")&&same(js_get(&vm,0,"templateLabel").text,"template"),320);
    uint32_t statement_bytes=0;const char *prefix="var work=0;",*statement="work=work+1;";
    while(*prefix)many_statements[statement_bytes++]=*prefix++;
    for(uint32_t i=0;i<5000;i++)for(const char *p=statement;*p;p++)many_statements[statement_bytes++]=*p;
    many_statements[statement_bytes]=0;
    js_init(&vm,host,0);CHECK(run(many_statements)&&js_get(&vm,0,"work").number==5000,321);
    js_init(&vm,host,0);
    CHECK(run("var items=Array(20001).fill(0);var iterations=0;items.forEach(()=>{iterations=iterations+1;});"),322);
    CHECK(js_get(&vm,0,"iterations").number==20001,323);
    js_init(&vm,host,0);
    CHECK(run("var total=0;for(var i=0;i<20001;i++){total+=1;}var bits=(0xffffffff>>>0);var kind=typeof missing;var base={value:7};var derived=Object.create(base);function Counter(n){this.value=n;}Counter.prototype.read=function(){return this.value;};var counter=new Counter(19);var answer=counter.read();var caught=false;try{throw new Error('test');}catch(e){caught=e.message==='test';}"),324);
    CHECK(js_get(&vm,0,"total").number==20001&&js_get(&vm,0,"bits").number==4294967295LL&&same(js_get(&vm,0,"kind").text,"undefined")&&js_get(&vm,0,"answer").number==19&&js_get(&vm,0,"caught").number==1,325);
    CHECK(run("var inherited=derived.value;var bound=counter.read();"),326);
    CHECK(js_get(&vm,0,"inherited").number==7&&js_get(&vm,0,"bound").number==19,327);
    CHECK(js_set(&vm,0,"twice",js_native(1))&&run("var sum=0;for(var j=0;j<3;j++){sum+=twice(j);}"),328);
    CHECK(js_get(&vm,0,"sum").number==6,329);
    js_value bridged=js_object(&vm);CHECK(js_set(&vm,0,"node",bridged),330);
    CHECK(run("var update=()=>{node.value=23;};"),331);
    CHECK(run("for(var k=0;k<1;k++){update();node.extra=31;}"),332);
    CHECK(js_get(&vm,bridged.number,"value").number==23&&js_get(&vm,bridged.number,"extra").number==31,333);
    CHECK(run("var clone=counter;"),334);
    CHECK(js_get(&vm,0,"clone").number==js_get(&vm,0,"counter").number,335);
    CHECK(run("var starts=0;function defaults(a,b=(starts+=1)){return this===globalThis?b:0;}var firstDefault=defaults(1);var secondDefault=defaults(1,9);var thirdDefault=defaults(1);"),336);
    CHECK(js_get(&vm,0,"firstDefault").number==1&&js_get(&vm,0,"secondDefault").number==9&&js_get(&vm,0,"thirdDefault").number==2&&js_get(&vm,0,"starts").number==2,337);
    CHECK(run("var escaped=escape('a b\\u0000\\u20ac');var unescaped=unescape('%00%FF%u20AC%zz');var validEscapes=escaped==='a%20b%00%u20AC'&&unescaped.charCodeAt(0)===0&&unescaped.charCodeAt(1)===255&&unescaped.charCodeAt(2)===8364&&unescaped.slice(3)==='%zz';"),338);
    CHECK(js_get(&vm,0,"validEscapes").number==1,339);
    CHECK(run("node.third=41;"),340);
    CHECK(run("var keys=Object.keys(node);var ownProperties=node.hasOwnProperty('third');var tally=0;for(var field in node){tally+=node[field];}"),341);
    CHECK(js_get(&vm,0,"ownProperties").number==1&&js_get(&vm,0,"tally").number==95,342);
    CHECK(run("const retained=13;var changes=0;var change=()=>{changes=changes+1;};"),343);
    CHECK(run("for(var step=0;step<3;step++){change();}var preserved=retained;"),344);
    CHECK(js_get(&vm,0,"changes").number==3&&js_get(&vm,0,"preserved").number==13,345);
    js_init(&vm,host,0);uint32_t literal_bytes=0;
    for(const char *p="var giant=`";*p;p++)many_statements[literal_bytes++]=*p;
    for(uint32_t i=0;i<9000;i++)many_statements[literal_bytes++]='x';
    for(const char *p="${7}`;var expanded=`${giant}${giant}`;";*p;p++)many_statements[literal_bytes++]=*p;
    many_statements[literal_bytes]=0;CHECK(run(many_statements),560);
    CHECK(js_get(&vm,0,"giant").length==9001&&js_get(&vm,0,"expanded").length==18002,561);
    js_init(&vm,host,0);
    CHECK(run("class Base { constructor(value=3){this.value=value;} get doubled(){return this.value*2;} } class Derived extends Base { #extra=5; static label='derived'; constructor(...args){super(...args);} read(){return this.value+this.#extra;} } var instance=new Derived(7);var classOK=instance instanceof Base&&instance instanceof Derived&&instance.read()===12&&instance.doubled===14&&Derived.label==='derived';"),562);
    CHECK(js_get(&vm,0,"classOK").number==1,563);
    CHECK(run("var order='';var asyncResult=0;async function calculate(){order+='start';var value=await Promise.resolve(7);order+='resume';return value+1;}var pending=calculate();order+='sync';pending.then(value=>{asyncResult=value;});"),564);
    CHECK(js_get(&vm,0,"asyncResult").number==8&&same(js_get(&vm,0,"order").text,"startsyncresume"),565);
    CHECK(run("var rejection='';async function fail(){throw new Error('async failure');}fail().catch(error=>{rejection=error.message;});var nested=`outer ${`inner ${3}`}`;"),566);
    CHECK(same(js_get(&vm,0,"rejection").text,"async failure")&&same(js_get(&vm,0,"nested").text,"outer inner 3"),567);
    return 0;
}
