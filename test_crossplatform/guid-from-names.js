var extension = require('../dtrace-provider');
console.log('required');

var generated_guid = extension.guidFromNames('provider_name', 'module_name');
console.log('generated guid 1 = ' + generated_guid);

generated_guid = extension.guidFromNames('provider_name');
console.log('generated guid 2 = ' + generated_guid);

process.exit(0);