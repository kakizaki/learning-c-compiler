#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
#include <stdio.h>
#include <stdlib.h>

void errorMessage(int e, int a) { 
  printf("%d expected, but got %d\n", e, a);
  exit(1);
}
EOF

./9cc testcc.9cc > tmp.s

if [ ! $? = 0 ]; then
  echo "! complie error !"
  faulted=1
else
  gcc -static -o tmp tmp.s tmp2.o
  ./tmp
fi

actual="$?"
echo "$actual"

echo "done"

