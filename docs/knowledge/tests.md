Test 1 : No Match

Input
BUY 100 @100
SELL 40 @105

Expected

BUY BOOK
100 ->100
SELL BOOK
105 ->40

----------------------------

Test 2 : Exact Match

BUY 100 @105
SELL 100 @105

Expected
Empty Book

----------------------------

Test 3 : Partial Fill

BUY 100 @105
SELL 40 @105

Expected

BUY BOOK
105 ->60

----------------------------

Test 4 : Cross Multiple Price Levels

SELL
20 @100
30 @101
40 @102

BUY
80 @102

Expected

100 removed
101 removed
102 ->10

----------------------------

Test 5 : FIFO

SELL
OrderA
50 @100
OrderB
50 @100

BUY
60 @100

Expected

OrderA removed
OrderB ->40