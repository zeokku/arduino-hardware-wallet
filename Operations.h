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
output: g[Status : 1 byte bool]{ [PublicKey : 32 bytes] | [FailReason : \0 terminated] }

__ sign payload __
input: s[Password]\0[PayloadLength : 2 bytes int][Payload : PayloadLength bytes]
output: s[Status : 1 byte bool]{ [Signature : 64 bytes] | [FailReason : \0 terminated] }

__ store key __
input: k[Password]\0[Key : 32 bytes]
output: k[Status : 1 byte bool]{ [FailReason : \0 terminated] }

@todo__ get public key __
input: p[Password]
output: p[Status : 1 byte bool]{ [PublicKey : 32 bytes] | [FailReason : \0 terminated] }

*/