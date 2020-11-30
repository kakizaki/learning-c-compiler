#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
#include <stdio.h>
int ret3() { return 3; }
int ret5() { return 5; }
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int printInt(int a) { printf("%x\n", a); return a; }
EOF


try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -static -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual >> $input"
    exit 1
  fi
}


dryrun() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -static -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual >> $input"
  fi
}


<< COMMENTOUT
try 1 'main() {1;}'
try 1 'main() {a=1;if(1)a;1;}'
try 1 'main() {a=1;if(1)2;a;}'
try 1 'main() {a=1;if(0)2;a;}'
try 1 'main() {a=1;{if(0)2;}a;}'
COMMENTOUT

try 0 'int main() {return 0;}'
try 42 'int main() {return 42;}'

try 21 'int main() {return 5+20-4;}'
try 41 'int main() {return  12 + 34 - 5 ;}'

try 47 'int main() {return 5+6*7;}'
try 15 'int main() {return 5*(9-6);}'
try 4 'int main() {return (3+5)/2;}'

try 10 'int main() {return -10+20;}'
try 10 'int main() {return -(5+5)+20;}'

try 0 'int main() {return 1==2;}'
try 1 'int main() {return 2==2;}'
try 0 'int main() {return 2!=2;}'
try 1 'int main() {return 1!=2;}'

try 0 'int main() {return 2<2;}'
try 1 'int main() {return 1<2;}'
try 0 'int main() {return 2>2;}'
try 1 'int main() {return 3>2;}'

try 0 'int main() {return 3<=2;}'
try 1 'int main() {return 2<=2;}'
try 1 'int main() {return 1<=2;}'
try 0 'int main() {return 1>=2;}'
try 1 'int main() {return 2>=2;}'
try 1 'int main() {return 3>=2;}'

try 1 'int main() {return 1*2 == 4/2;}'
try 1 'int main() {return 1*2 == (2+2)/(1+1);}'


# 複文
try 2 'int main() {0;1; return 2;}'
try 3 'int main() {10+20;11*11; return 1+2;}'

# 変数
try 3 'int main() {int a;return a=1+2;}'
try 3 'int main() {int a;return a=1+2*3-8/2;}'

try 3 'int main() {int a;int b;a=1;b=2;return a+b;}'
try 0 'int main() {int a;int b;int c;a=1;b=2;return c=4/b-a*2;}'

# 変数 2
try 3 'int main() {int abc;return abc=1+2;}'
try 3 'int main() {int abc;return abc=1+2*3-8/2;}'

try 3 'int main() {int abc;int bzz;abc=1;bzz=2;return abc+bzz;}'
try 0 'int main() {int abc;int bzz;int ab;abc=1;bzz=2;return ab=4/bzz-abc*2;}'

try 3 'int main() {int _a12;int __b__3;_a12=1;__b__3=2;return _a12+__b__3;}'


# return
try 4 'int main() {int abc;abc=1+2;return 4;}'
try 3 'int main() {int abc;abc=1+2*3-8/2;return abc;}'

try 3 'int main() {int abc;int bzz;abc=1;bzz=2;return abc+bzz;}'
try 0 'int main() {int abc;int bzz;int ab;abc=1;bzz=2;return ab=4/bzz-abc*2;}'

try 3 'int main() {int _a12;int __b__3;_a12=1;__b__3=2;return _a12+__b__3;}'
try 2 'int main() {int returnx;returnx=2; return returnx;}'

# if
try 2 'int main() {if(1)return 2;return 3;}'
try 3 'int main() {if(0)return 2;return 3;}'
try 2 'int main() {int a;a=0;if(a+1)return 2;return 3;}'

try 2 'int main() {if(1)if(1)return 2;return 3;}'
try 3 'int main() {if(1)if(0)return 2;return 3;}'
try 3 'int main() {if(0)if(1)return 2;return 3;}'


# if else
echo "### if else"
try 2 'int main() {if(1)return 2;else return 3;}'
try 3 'int main() {if(0)return 2;else return 3;}'
try 2 'int main() {int a;a=0;if(a+1)return 2;else return 3;}'

try 2 'int main() {if(1)if(1)return 2;else return 3;}'
try 3 'int main() {if(1)if(0)return 2;else return 3;}'
try 4 'int main() {if(0)if(1)return 2;else return 3;else return 4;}'

# while
echo "### while"
try 10 'int main() {int a;a=1;while(a<10)a=a+1;return a;}'
try 9 'int main() {int a;a=1;while(a+1<10)a=a+1;return a;}'
try 1 'int main() {int a;a=1;while(a<0)a=a+1;return a;}'

# for
echo "### for"
try 45 'int main() {int a;int b;b=0;for(a=1;a<10;a=a+1)b=b+a;return b;}'
try 10 'int main() {int a;a=0;for(;a<10;)a=a+1;return a;}'

# {}
echo "### {}"
try 3 'int main() {{1;{2;return 3;}}}'
try 2 'int main() {if (1) {1;return 2;}}'
try 4 'int main() {if (0) {1;2;} else {3; return 4;}}'

try 3 'int main() {{1; {2;} return 3;}}'
try 55 'int main() {int i;int j;i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j;}'

#
echo "### function call"
try 3 'int main() {return ret3();}'
try 5 'int main() {return ret5();}'
try 10 'int main() {return add(3, 7);}'
try 11 'int main() {return add(add(1,3), 7);}'
try 12 'int main() {int a;a=1;return add(add(a,4), 7);}'
try 2 'int main() {return sub(3, 1);}'

#
echo "### function"
try 3 'int main() { int a;a=1; return a + f();} int f() { int a;a=2; return a;}'
try 2 'int main() { return f(1);} int f(int a) { return a + 1; }'
try 3 'int main() { return f(3, 1);} int f(int a, int b) { return a; }'
try 1 'int main() { return f(3, 1);} int f(int a, int b) { return b; }'
try 7 'int main() { return add2(3, 4);} int add2(int a, int b) { return a+b; }'
try 1 'int main() { return sub2(4, 3);} int sub2(int a, int b) { return a-b; }'
try 55 'int main() { return fib(9);} int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

#
echo "### & , * "
try 1 'int main() { int a;int b;a = 1; b = &a; return *b; }'
try 3 'int main() { int a;int b;int c;a = 3; b = 5; c = &b - 1; return *c; }'
try 4 'int main() { int a;int b;a = 3; b = 1 + *(&a); return b; }'

try 3 'int main() {int a;a=3;return *&a;}'
try 0 'int main() {int *a;int **b;int ***c; return 0;}'

try 1 'int main() {int a;int *b;a = 1;b = &a;return *b;}'
try 3 'int main() {int a;int *b;b = &a; *b = 3;return a;}'

try 7 'int main() {int a;int b;a=3;b=5;*(&a+1)=7;return b;}'
try 2 'int main() {int a;int b;int *c;c=&a;*(c+1)=2;return b;}'

# スタックに確保した変数のアドレスが、確保した順に昇順になるようにする
dryrun 1 'int main() {int a;int b;printInt(&a);printInt(&b);return 1;}'
try 1 'int main() {int a;int b; return &a < &b;}'

#
echo "### pointer arithmetic "
try 5 'int main() { int x=3; int y=5; return *(&x+1); }'
try 3 'int main() { int x=3; int y=5; return *(&y-1); }'
try 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
try 7 'int main() { int x=3; int y=5; *(&y-1)=7; return x; }'
try 2 'int main() { int x; return (&x+2)-&x; }'

#
echo "### sizeof"
# 現状、パディングありのため int は 8 バイト扱い
#try 4 'int main() { int a; return sizeof(a); }'
try 8 'int main() { int a; return sizeof(a); }'
try 8 'int main() { int *a; return sizeof(a); }'
#try 4 'int main() { return sizeof(1); }'
try 8 'int main() { return sizeof(1); }'
#try 4 'int main() { return sizeof(sizeof(1)); }'
try 8 'int main() { return sizeof(sizeof(1)); }'
try 8 'int main() { int a;return sizeof(&a+1); }'

#
echo '### array' 
try 4 'int main() {int a[2];*a=1;*(a+1)=3;return *a+*(a+1);}'
try 3 'int main() {int a[2];*a=1;*(a+1)=2;int *p;return *a+*(a+1);}'

try 3 'int main() {int x[2];int *y=&x; *y=3; return *x; }'
try 3 'int main() {int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
try 4 'int main() {int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
try 5 'int main() {int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

#
echo '### array indexer'
try 1 'int main() {int a[2];int i;a[0]=1;return a[0];}'
try 1 'int main() {int a[2];int i;a[1]=1;return a[1];}'
try 1 'int main() {int a[2];int i;a[0+1]=1;return a[1];}'
try 1 'int main() {int a[2];int i=1;a[i]=1;return a[1];}'
try 1 'int main() {int a[2];int i1=0;int i2=1;a[i1+i2]=1;return a[1];}'
try 45 'int main() {int a[10];int i=0;int sum=0;for(i=0;i<10;i=i+1) {a[i]=i;sum=sum+a[i];} return sum;}'

#
echo '### array sizeof'
# 現状、パディングありのため int は 8 バイト扱い
try 40 'int main() {int a[5];return sizeof(a);}'


#
echo '### global variable'
try 0 'int x; int main() { return x; }'
try 3 'int x; int main() { x=3; return x; }'
try 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
try 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
try 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
try 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'

try 8 'int x; int main() { return sizeof(x); }'
try 32 'int x[4]; int main() { return sizeof(x); }'

try 6 'int a;int main() {int b = 2;a = 3;return a * b;}'
try 6 'int a;int b;int main() {b = 2;a = 3;return a * b;}'
try 12 'int a;int main() {a=12;f2();return a;} int f2(){int a=2;return 1;}'



echo OK
