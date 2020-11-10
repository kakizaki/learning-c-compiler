#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

try 0 '0;'
try 42 '42;'

try 21 '5+20-4;'
try 41 ' 12 + 34 - 5 ;'

try 47 '5+6*7;'
try 15 '5*(9-6);'
try 4 '(3+5)/2;'

try 10 '-10+20;'
try 10 '-(5+5)+20;'

try 0 '1==2;'
try 1 '2==2;'
try 0 '2!=2;'
try 1 '1!=2;'

try 0 '2<2;'
try 1 '1<2;'
try 0 '2>2;'
try 1 '3>2;'

try 0 '3<=2;'
try 1 '2<=2;'
try 1 '1<=2;'
try 0 '1>=2;'
try 1 '2>=2;'
try 1 '3>=2;'

try 1 '1*2 == 4/2;'
try 1 '1*2 == (2+2)/(1+1);'


# 複文
try 2 '0;1;2;'
try 3 '10+20;11*11;1+2;'

# 変数
try 3 'a=1+2;'
try 3 'a=1+2*3-8/2;'

try 3 'a=1;b=2;a+b;'
try 0 'a=1;b=2;c=4/b-a*2;'

# 変数 2
try 3 'abc=1+2;'
try 3 'abc=1+2*3-8/2;'

try 3 'abc=1;bzz=2;abc+bzz;'
try 0 'abc=1;bzz=2;ab=4/bzz-abc*2;'

try 3 '_a12=1;__b__3=2;_a12+__b__3;'

# return
try 4 'abc=1+2;return 4;'
try 3 'abc=1+2*3-8/2;return abc;'

try 3 'abc=1;bzz=2;return abc+bzz;'
try 0 'abc=1;bzz=2;return ab=4/bzz-abc*2;'

try 3 '_a12=1;__b__3=2;return _a12+__b__3;'

try 2 'returnx=2; return returnx;'



echo OK
