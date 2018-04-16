# CH.A.S.M.
CHaracter Aware Split Method

The main idea behind Chasm is to reuse password patterns in a way that is likely to produce probable password candidates.

## So how does this work?

![chasm1](https://user-images.githubusercontent.com/7359229/38832202-6fcb8316-4187-11e8-853c-25fef9035e4a.jpg)
![chasm2](https://user-images.githubusercontent.com/7359229/38832203-6fdf4f7c-4187-11e8-89bf-b609429e2723.jpg)
![chasm3](https://user-images.githubusercontent.com/7359229/38832204-6ff46eb6-4187-11e8-81f2-e75f8ed43571.jpg)
![chasm4](https://user-images.githubusercontent.com/7359229/38832205-70064fbe-4187-11e8-92cc-fea5531072af.jpg)

### Chasm usage
`chasm -l 30 -k 2 < dictionary.txt`

## Some user definable options that Chasm supports:
- -a (Analyze frequency, will output the frequency of the split part)
- -c Create rules for dictionary attack eg left-side becomes prefix rules ^ conversely right-side becomes $ suffix rules
- -s Sort by frequency, highest first
- -o [charcode] Only splits if the split character matches the character code, eg -o 101 will only split on the letter 'e' at the defined midpoint/range
- -l [string len] Will not split if the length of the input string if greater than this number -l 20 will skip all strings greater than length 20. 
- -k [min occurance] Will not write splits where occurrence is less than defined occurrence -k 2 will not output splits which only occur once
- -m [mid point] Instead of splitting the string in half (len/2) you can specify a position to perform the split on
- -r [number] Will split X chars around the middle including the middle, X denotes how far you want to branch out
