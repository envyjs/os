// Windows Script Host build utility. Generates NASM ISO metadata, never executes UI code.
var fso = new ActiveXObject('Scripting.FileSystemObject');
try {
    if (WScript.Arguments.length < 2 || WScript.Arguments.length > 3) throw new Error('Usage: cscript envy.js root output.inc [image-previews]');
    var root = fso.GetAbsolutePathName(WScript.Arguments(0));
    var output = fso.GetAbsolutePathName(WScript.Arguments(1));
    var previews = WScript.Arguments.length == 3 ? fso.GetAbsolutePathName(WScript.Arguments(2)) : '';
    var dirs = [], files = [], next = 128, lines = [], stamps = [];
    var excluded = {kernel:1, tools:1, build:1, platforms:1, makefile:1};
    function quote(s) {
        if (s.indexOf("'") >= 0 || /[^\x20-\x7e]/.test(s)) throw new Error('Unsupported image filename: ' + s);
        return "'" + s.replace(/\\/g, '/') + "'";
    }
    function name(s, directory) {
        var n = s.toUpperCase() + (directory ? '' : ';1');
        if (n.length > 207) throw new Error('ISO identifier too long: ' + s);
        quote(n); return n;
    }
    function entries(folder, top) {
        var list = [], e;
        for (e = new Enumerator(folder.SubFolders); !e.atEnd(); e.moveNext()) list.push({file:e.item(), dir:true});
        for (e = new Enumerator(folder.Files); !e.atEnd(); e.moveNext()) list.push({file:e.item(), dir:false});
        list.sort(function(a,b) { var x=a.file.Name.toUpperCase(), y=b.file.Name.toUpperCase(); return x<y?-1:x>y?1:0; });
        var result=[], previous='';
        for (var i=0;i<list.length;i++) {
            var item=list[i], n=item.file.Name;
            if (top && (n.charAt(0)=='.' || excluded[n.toLowerCase()])) continue;
            if (!item.dir && /^(readme|security)(\..*)?$/i.test(n)) continue;
            if (item.file.Attributes & 1024) throw new Error('Reparse points are not packaged: '+item.file.Path);
            item.iso=name(n,item.dir);
            if (item.iso==previous) throw new Error('ISO name collision: '+n);
            previous=item.iso; result.push(item);
        }
        return result;
    }
    var envy={path:root, iso:'ENVY', parent:1, number:3, children:entries(fso.GetFolder(root),true)};
    dirs.push(envy);
    for (var d=0;d<dirs.length;d++) {
        var dir=dirs[d];
        for (var i=0;i<dir.children.length;i++) {
            var item=dir.children[i];
            if (item.dir) {
                item.node={path:item.file.Path, iso:item.iso, parent:dir.number, number:dirs.length+3, children:entries(item.file,false)};
                dirs.push(item.node);
            } else {
                var packaged=item.file;
                if(previews) {
                    var preview=previews+'\\'+item.file.Path.substr(root.length+1);
                    if(fso.FileExists(preview))packaged=fso.GetFile(preview);
                }
                item.node={path:packaged.Path, iso:item.iso, size:Number(packaged.Size)};
                if (item.node.size>0xffffffff) throw new Error('File larger than single-extent ISO limit');
                files.push(item.node);
                stamps.push('// '+item.file.Path+' '+item.file.Size+' '+Number(new Date(item.file.DateLastModified)));
                if(packaged.Path!=item.file.Path)stamps.push('// '+packaged.Path+' '+packaged.Size+' '+Number(new Date(packaged.DateLastModified)));
            }
        }
    }
    function recordSize(n) { return 33+n.length+((n.length%2)==0?1:0); }
    for (var d=0;d<dirs.length;d++) {
        var dir=dirs[d], used=68;
        for(var i=0;i<dir.children.length;i++) {
            var size=recordSize(dir.children[i].iso);
            if (used%2048+size>2048) used+=2048-used%2048;
            used+=size;
        }
        dir.size=Math.ceil(used/2048)*2048; dir.lba=next; next+=dir.size/2048;
    }
    for(var i=0;i<files.length;i++) { files[i].lba=next; next+=Math.max(1,Math.ceil(files[i].size/2048)); }
    var tableBytes=22;
    for(var d=0;d<dirs.length;d++) tableBytes+=8+dirs[d].iso.length+(dirs[d].iso.length%2);
    var little=next; next+=Math.ceil(tableBytes/2048);
    var big=next; next+=Math.ceil(tableBytes/2048);
    lines.push('; Generated file. Source file timestamps make edits rebuild the ISO.');
    for(var i=0;i<stamps.length;i++) lines.push(';'+stamps[i]);
    lines.push('%undef ISO_BLOCKS','%define ISO_BLOCKS '+next,'%define ENVY_LBA '+envy.lba,'%define ENVY_SIZE '+envy.size,
               '%define PATH_BYTES '+tableBytes,'%define L_TABLE '+little,'%define M_TABLE '+big,'%macro envy_body 0');
    function rec(node, flags, identifier) { lines.push('directory_record '+node.lba+', '+node.size+', '+flags+', '+identifier); }
    for(var d=0;d<dirs.length;d++) {
        var dir=dirs[d], used=68;
        lines.push('block '+dir.lba);
        rec(dir,2,'0'); rec(d==0?{lba:22,size:2048}:dirs[dir.parent-3],2,'1');
        for(var i=0;i<dir.children.length;i++) {
            var item=dir.children[i], size=recordSize(item.iso);
            if (used%2048+size>2048) { used+=2048-used%2048; lines.push('block '+(dir.lba+used/2048)); }
            rec(item.node,item.dir?2:0,quote(item.iso)); used+=size;
        }
    }
    for(var i=0;i<files.length;i++) { lines.push('block '+files[i].lba,'incbin '+quote(files[i].path)); }
    function table(lba, bigEndian) {
        lines.push('block '+lba,'db 1, 0',bigEndian?'be32 22':'dd 22',bigEndian?'be16 1':'dw 1','db 0, 0',
                   'db 4, 0',bigEndian?'be32 23':'dd 23',bigEndian?'be16 1':'dw 1',"db 'DOCS'");
        for(var d=0;d<dirs.length;d++) {
            var dir=dirs[d]; lines.push('db '+dir.iso.length+', 0',(bigEndian?'be32 ':'dd ')+dir.lba,
                         (bigEndian?'be16 ':'dw ')+dir.parent,'db '+quote(dir.iso));
            if (dir.iso.length%2) lines.push('db 0');
        }
    }
    table(little,false); table(big,true); lines.push('block '+next,'%endmacro');
    var text=lines.join('\r\n')+'\r\n', old='';
    if(fso.FileExists(output)) { var input=fso.OpenTextFile(output,1); old=input.ReadAll(); input.Close(); }
    if(text!=old) { var out=fso.CreateTextFile(output,true,false); out.Write(text); out.Close(); }
    WScript.Echo('Envy image: '+files.length+' files, '+dirs.length+' directories, '+next+' ISO blocks.');
} catch(error) { WScript.Echo('ERROR: '+error.message); WScript.Quit(1); }
