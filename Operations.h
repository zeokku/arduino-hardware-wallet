/*
it needs to:
 store key,
 generate signature,
 export key,

 generate key
*/

/*
gen key -> encrypt, store
*/

/*

__ generate key __
input: g[Password]
output: g[Status : 1 byte bool][PublicKey : 32 bytes]

__ sign payload __
input: s[Password]\0[PayloadLength : 2 bytes int][Payload : PayloadLength bytes]
output: s[Status : 1 byte bool][Signature : 64 bytes]

*/