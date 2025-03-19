define i1 @and(i1 %a,i1 %b){
  %c = and i1 %a,%b
  ret i1 %c
}
define i1 @nand(i1 %a,i1 %b){
  %1 = and i1 %a,%b
  %2 = xor i1 %1, true
  ret i1 %2
}
define i1 @not(i1 %a){
  %c = xor i1 %a, true
  ret i1 %c
}
define i1 @xnor(i1 %a,i1 %b){
  %c = xor i1 %a, %b
  %d = xor i1 %c, true
  ret i1 %d
}
define i1 @xor(i1 %a,i1 %b){
  %c = xor i1 %a, %b
  ret i1 %c
}
define i1 @buffer(i1 %a){
  ret i1 %a
}
define i1 @nor(i1 %a,i1 %b){
  %c = or i1 %a,%b
  %1 = xor i1 %c, true
  ret i1 %1
}
define i1 @or(i1 %a,i1 %b){
  %c = or i1 %a,%b
  ret i1 %c
}
define i1 @ite(i1 %a,i1 %b,i1 %c){
  %1 = select i1 %a,i1 %b,i1 %c
  ret i1 %1
}
