#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
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
    echo "$expected expected, but got $actual >> $input"
    exit 1
  fi
}

<< COMMENTOUT
try 1 'main() {1;}'
try 1 'main() {a=1;if(1)a;1;}'
try 1 'main() {a=1;if(1)2;a;}'
try 1 'main() {a=1;if(0)2;a;}'
try 1 'main() {a=1;{if(0)2;}a;}'
COMMENTOUT


try 0 'main() {return 0;}'
try 42 'main() {return 42;}'

try 21 'main() {return 5+20-4;}'
try 41 'main() {return  12 + 34 - 5 ;}'

try 47 'main() {return 5+6*7;}'
try 15 'main() {return 5*(9-6);}'
try 4 'main() {return (3+5)/2;}'

try 10 'main() {return -10+20;}'
try 10 'main() {return -(5+5)+20;}'

try 0 'main() {return 1==2;}'
try 1 'main() {return 2==2;}'
try 0 'main() {return 2!=2;}'
try 1 'main() {return 1!=2;}'

try 0 'main() {return 2<2;}'
try 1 'main() {return 1<2;}'
try 0 'main() {return 2>2;}'
try 1 'main() {return 3>2;}'

try 0 'main() {return 3<=2;}'
try 1 'main() {return 2<=2;}'
try 1 'main() {return 1<=2;}'
try 0 'main() {return 1>=2;}'
try 1 'main() {return 2>=2;}'
try 1 'main() {return 3>=2;}'

try 1 'main() {return 1*2 == 4/2;}'
try 1 'main() {return 1*2 == (2+2)/(1+1);}'


# 複文
try 2 'main() {0;1; return 2;}'
try 3 'main() {10+20;11*11; return 1+2;}'

# 変数
try 3 'main() {return a=1+2;}'
try 3 'main() {return a=1+2*3-8/2;}'

try 3 'main() {a=1;b=2;return a+b;}'
try 0 'main() {a=1;b=2;return c=4/b-a*2;}'

# 変数 2
try 3 'main() {return abc=1+2;}'
try 3 'main() {return abc=1+2*3-8/2;}'

try 3 'main() {abc=1;bzz=2;return abc+bzz;}'
try 0 'main() {abc=1;bzz=2;return ab=4/bzz-abc*2;}'

try 3 'main() {_a12=1;__b__3=2;return _a12+__b__3;}'


# return
try 4 'main() {abc=1+2;return 4;}'
try 3 'main() {abc=1+2*3-8/2;return abc;}'

try 3 'main() {abc=1;bzz=2;return abc+bzz;}'
try 0 'main() {abc=1;bzz=2;return ab=4/bzz-abc*2;}'

try 3 'main() {_a12=1;__b__3=2;return _a12+__b__3;}'
try 2 'main() {returnx=2; return returnx;}'

# if
try 2 'main() {if(1)return 2;return 3;}'
try 3 'main() {if(0)return 2;return 3;}'
try 2 'main() {a=0;if(a+1)return 2;return 3;}'

try 2 'main() {if(1)if(1)return 2;return 3;}'
try 3 'main() {if(1)if(0)return 2;return 3;}'
try 3 'main() {if(0)if(1)return 2;return 3;}'


# if else
echo "### if else"
try 2 'main() {if(1)return 2;else return 3;}'
try 3 'main() {if(0)return 2;else return 3;}'
try 2 'main() {a=0;if(a+1)return 2;else return 3;}'

try 2 'main() {if(1)if(1)return 2;else return 3;}'
try 3 'main() {if(1)if(0)return 2;else return 3;}'
try 4 'main() {if(0)if(1)return 2;else return 3;else return 4;}'

# while
echo "### while"
try 10 'main() {a=1;while(a<10)a=a+1;return a;}'
try 9 'main() {a=1;while(a+1<10)a=a+1;return a;}'
try 1 'main() {a=1;while(a<0)a=a+1;return a;}'

# for
echo "### for"
try 45 'main() {b=0;for(a=1;a<10;a=a+1)b=b+a;return b;}'
try 10 'main() {a=0;for(;a<10;)a=a+1;return a;}'

# {}
echo "### {}"
try 3 'main() {{1;{2;return 3;}}}'
try 2 'main() {if (1) {1;return 2;}}'
try 4 'main() {if (0) {1;2;} else {3; return 4;}}'

try 3 'main() {{1; {2;} return 3;}}'
try 55 'main() {i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j;}'

#
echo "### function call"
try 3 'main() {return ret3();}'
try 5 'main() {return ret5();}'
try 10 'main() {return add(3, 7);}'
try 11 'main() {return add(add(1,3), 7);}'
try 12 'main() {a=1;return add(add(a,4), 7);}'
try 2 'main() {return sub(3, 1);}'

#
echo "### function"
try 3 'main() { a=1; return a + f();} f() { a=2; return a;}'
try 2 'main() { return f(1);} f(a) { return a + 1; }'
try 3 'main() { return f(3, 1);} f(a, b) { return a; }'
try 1 'main() { return f(3, 1);} f(a, b) { return b; }'
try 7 'main() { return add2(3, 4);} add2(a, b) { return a+b; }'
try 1 'main() { return sub2(4, 3);} sub2(a, b) { return a-b; }'
try 55 'main() { return fib(9);} fib(x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

echo OK
