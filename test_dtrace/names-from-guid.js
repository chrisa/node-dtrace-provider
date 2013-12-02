var provider = require('../dtrace-provider');
console.log('required');

var generated_names = provider.namesFromGuid('5A391F32-A079-42E1-97B1-8A45295AA1FD');
console.log('generated provider name = ' + generated_names[0]);
console.log('generated module name = ' + generated_names[1]);

process.exit(0);