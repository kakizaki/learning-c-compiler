#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
EOF

try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s tmp2.o
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

# if
try 2 'if(1)return 2;return 3;'
try 3 'if(0)return 2;return 3;'
try 2 'a=0;if(a+1)return 2;return 3;'

try 2 'if(1)if(1)return 2;return 3;'
try 3 'if(1)if(0)return 2;return 3;'
try 3 'if(0)if(1)return 2;return 3;'

# if else
echo "### if else"
try 2 'if(1)return 2;else return 3;'
try 3 'if(0)return 2;else return 3;'
try 2 'a=0;if(a+1)return 2;else return 3;'

try 2 'if(1)if(1)return 2;else return 3;'
try 3 'if(1)if(0)return 2;else return 3;'
try 4 'if(0)if(1)return 2;else return 3;else return 4;'

# while
echo "### while"
try 10 'a=1;while(a<10)a=a+1;return a;'
try 9 'a=1;while(a+1<10)a=a+1;return a;'
try 1 'a=1;while(a<0)a=a+1;return a;'

# for
echo "### for"
try 45 'b=0;for(a=1;a<10;a=a+1)b=b+a;return b;'
try 10 'a=0;for(;a<10;)a=a+1;return a;'

# {}
echo "### {}"
try 1 '{1;}'
try 3 '{1;{2;return 3;}}'
try 2 'if (1) {1;2;}'
try 4 'if (0) {1;2;} else {3;4;}'

try 3 '{1; {2;} return 3;}'
try 55 'i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j;'

#
echo "### function call"
try 3 'return ret3();'
try 5 'return ret5();'

echo OK
