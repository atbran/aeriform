from pathlib import Path
def emit(rows,effects,root):
 root=Path(root)
 def write(name,text):(root/name).write_text(text,encoding='utf-8',newline='\n')
 mods=[r for r in rows if r['enum'].startswith(('rack','breath')) and r['kind']=='Float']
 write('Source/Params/AdvancedModEnums.inc',''.join('    '+r['enum']+',\n' for r in mods))
 write('Source/Params/AdvancedModNames.inc',''.join('    "'+r['name']+'",\n' for r in mods))
 bindings='#pragma once\n#include "ParameterLayout.h"\nnamespace aeriform {\nstruct AdvancedBinding { P parameter; ModDest destination; float lo,hi; };\ninline constexpr AdvancedBinding advancedBindings[] = {\n'
 bindings+=''.join('    {P::%s,ModDest::%s,%sf,%sf},\n'%(r['enum'],r['enum'],float(r['lo']),float(r['hi'])) for r in mods)
 bindings+=' };\ninline ModDest advancedDestination(P p) {for(auto b:advancedBindings)if(b.parameter==p)return b.destination;return ModDest::None;}\n}\n'
 write('Source/Params/AdvancedBindings.h',bindings)
 fields=[r for r in effects if r['enum'] not in ('rdOn','shOn','sfOn','satOn')]
 text='#pragma once\n#include "AdvancedBindings.h"\nnamespace aeriform {\ninline constexpr P rackTypes[]{P::rack1Type,P::rack2Type,P::rack3Type,P::rack4Type};\ninline constexpr P rackEnables[]{P::rack1Enabled,P::rack2Enabled,P::rack3Enabled,P::rack4Enabled};\n'
 text+='inline constexpr P rackFields[4][%d] = {\n'%len(fields)
 for slot in range(1,5):text+=' {'+','.join('P::rack'+str(slot)+r['enum'][0].upper()+r['enum'][1:] for r in fields)+'},\n'
 text+='};\ninline constexpr P originalRackFields[] = {'+','.join('P::'+r['enum'] for r in fields)+'};\n'
 text+='inline std::array<int,4> rackPermutation(int code) {std::array<int,4> order{0,1,2,3};for(int i=0;i<std::clamp(code,0,23);++i)std::next_permutation(order.begin(),order.end());return order;}\n'
 text+='inline int rackOrderCode(std::array<int,4> order) {std::array<int,4> test{0,1,2,3};int code=0;do{if(test==order)return code;++code;}while(std::next_permutation(test.begin(),test.end()));return 0;}\n}\n'
 write('Source/Params/RackParameters.h',text)
