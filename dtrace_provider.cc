#include "dtrace_provider.h"
#include <v8.h>

#include <node.h>

namespace node {
  
  using namespace v8;
  
  void DTraceProvider::Initialize(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(DTraceProvider::New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    t->SetClassName(String::NewSymbol("DTraceProvider"));

    NODE_SET_PROTOTYPE_METHOD(t, "addProbe", DTraceProvider::AddProbe);
    NODE_SET_PROTOTYPE_METHOD(t, "enable", DTraceProvider::Enable);
    NODE_SET_PROTOTYPE_METHOD(t, "fire", DTraceProvider::Fire);

    target->Set(String::NewSymbol("DTraceProvider"), t->GetFunction());
  }
  
  Handle<Value> DTraceProvider::New(const Arguments& args) {
    HandleScope scope;
    DTraceProvider *p = new DTraceProvider();

    if (args.Length() != 1 || !args[0]->IsString()) {
      return ThrowException(Exception::Error(String::New(
        "Must give provider name as argument")));
    }
    
    String::AsciiValue name(args[0]->ToString());
    p->name = strdup(*name);

    p->Wrap(args.Holder());
    return args.This();
  }

  Handle<Value> DTraceProvider::AddProbe(const Arguments& args) {
    HandleScope scope;
    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(args.Holder());

    if (!args[0]->IsString()) {
      return ThrowException(Exception::Error(String::New(
        "Must give probe name as first argument")));
    }

    DTraceProbeDef *probe = new DTraceProbeDef();
    probe->next = NULL;

    // init argument types
    int i;
    for (i = 0; (args[i+1]->IsString() && i < 6); i++) {
      String::AsciiValue type(args[i+1]->ToString());
      probe->types[i] = strdup(*type);
    }
    probe->types[i] = NULL;

    // init name and function
    String::AsciiValue name(args[0]->ToString());
    probe->name = strdup(*name);
    probe->function = (char *) "func";

    // append to probe list
    if (provider->probe_defs == NULL)
      provider->probe_defs = probe;
    else {
      DTraceProbeDef *p;
      for (p = provider->probe_defs; (p->next != NULL); p = p->next) ;
      p->next = probe;
    }

    return Undefined();
  }

  static void *create_probe(int argc) {
    
#ifdef __APPLE__	
#define FUNC_SIZE 32
#define IS_ENABLED_FUNC_LEN 12
#else
#define FUNC_SIZE 96
#define IS_ENABLED_FUNC_LEN 32
#endif
    
#ifdef __APPLE__
    void *p = (void *) valloc(FUNC_SIZE);
    (void)mprotect((void *)p, FUNC_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);

    uint8_t tracepoints[FUNC_SIZE] = {
      0x55, 0x48, 0x89, 0xe5, 
      0x48, 0x33, 0xc0, 0x90, 
      0x90, 0xc9, 0xc3, 0x00,
      0x55, 0x48, 0x89, 0xe5, 
      0x90, 0x0f, 0x1f, 0x40, 
      0x00, 0xc9, 0xc3, 0x0f, 
      0x1f, 0x44, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00
    };

    memcpy(p, tracepoints, FUNC_SIZE);

#else // Solaris 32 bit

    void *p = (void *) memalign(PAGESIZE, FUNC_SIZE);
    (void)mprotect((void *)p, FUNC_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);

    uint8_t is_enabled[FUNC_SIZE] = {
      0x55, 0x89, 0xe5, 0x83,
      0xec, 0x08, 0x33, 0xc0,
      0x90, 0x90, 0x90, 0x89,
      0x45, 0xfc, 0x83, 0x7d,
      0xfc, 0x00, 0x0f, 0x95,
      0xc0, 0x0f, 0xb6, 0xc0,
      0x89, 0x45, 0xfc, 0x8b,
      0x45, 0xfc, 0xc9, 0xc3,
    };
    memcpy(p, is_enabled, FUNC_SIZE);
    
    switch(argc) {
    case 0:
      {
	uint8_t probe[FUNC_SIZE] = {
	  0x55, 0x89, 0xe5, 0x83,
	  0xec, 0x08, 0x90, 0x90,
	  0x90, 0x90, 0x90, 0x83,
	  0xc4, 0x00, 0xc9, 0xc3
	};
	memcpy((uint8_t *)p + 32, probe, FUNC_SIZE - 32);
      }
      break;
    case 1:
      {
	uint8_t probe[FUNC_SIZE] = {
	  0x55, 0x89, 0xe5, 0x83,
	  0xec, 0x08, 0xff, 0x75,
	  0x08, 0x90, 0x90, 0x90,
	  0x90, 0x90, 0x83, 0xc4,
	  0x00, 0xc9, 0xc3
	};
	memcpy((uint8_t *)p + 32, probe, FUNC_SIZE - 32);
      }
      break;
    case 2:
      {
	uint8_t probe[FUNC_SIZE] = {
	  0x55, 0x89, 0xe5, 0x83,
	  0xec, 0x08, 0xff, 0x75,
	  0x0c, 0xff, 0x75, 0x08,
	  0x90, 0x90, 0x90, 0x90,
	  0x90, 0x83, 0xc4, 0x00,
	  0xc9, 0xc3
	};
	memcpy((uint8_t *)p + 32, probe, FUNC_SIZE - 32);
      }
      break;
    case 3:
      {
	uint8_t probe[FUNC_SIZE] = {
	  0x55, 0x89, 0xe5, 0x83,
	  0xec, 0x08, 0xff, 0x75,
	  0x10, 0xff, 0x75, 0x0c,
	  0xff, 0x75, 0x08, 0x90,
	  0x90, 0x90, 0x90, 0x90,
	  0x83, 0xc4, 0x00, 0xc9,
	  0xc3
	};
	memcpy((uint8_t *)p + 32, probe, FUNC_SIZE - 32);
      }
      break;
    case 4:
      {
	uint8_t probe[FUNC_SIZE] = {
	  0x55, 0x89, 0xe5, 0x83,
	  0xec, 0x08, 0xff, 0x75,
	  0x14, 0xff, 0x75, 0x10,
	  0xff, 0x75, 0x0c, 0xff, 
	  0x75, 0x08, 0x90, 0x90,
	  0x90, 0x90, 0x90, 0x83,
	  0xc4, 0x00, 0xc9, 0xc3
	};
	memcpy((uint8_t *)p + 32, probe, FUNC_SIZE - 32);
      }
    case 5:
      {
	uint8_t probe[FUNC_SIZE] = {
	  0x55, 0x89, 0xe5, 0x83,
	  0xec, 0x08, 0xff, 0x75,
	  0x18, 0xff, 0x75, 0x14,
	  0xff, 0x75, 0x10, 0xff,
	  0x75, 0x0c, 0xff, 0x75,
	  0x08, 0x90, 0x90, 0x90,
	  0x90, 0x90, 0x83, 0xc4,
	  0x00, 0xc9, 0xc3
	};
	memcpy((uint8_t *)p + 32, probe, FUNC_SIZE - 32);
      }
    case 6:
      {
	uint8_t probe[FUNC_SIZE] = {
	  0x55, 0x89, 0xe5, 0x83,
	  0xec, 0x08, 0xff, 0x75,
	  0x1c, 0xff, 0x75, 0x18,
	  0xff, 0x75, 0x14, 0xff,
	  0x75, 0x10, 0xff, 0x75,
	  0x0c, 0xff, 0x75, 0x08,
	  0x90, 0x90, 0x90, 0x90,
	  0x90, 0x83, 0xc4, 0x00,
	  0xc9, 0xc3
	};
	memcpy((uint8_t *)p + 32, probe, FUNC_SIZE - 32);
      }
      break;
    }
#endif

    return p;
  }

  Handle<Value> DTraceProvider::Enable(const Arguments& args) {
    HandleScope scope;
    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(args.Holder());

    DOFStrtab *strtab = new DOFStrtab(0);
    dof_stridx_t pv_name = strtab->Add(provider->name);

    uint32_t argidx = 0;
    uint32_t offidx = 0;

    DOFSection *probes = new DOFSection(DOF_SECT_PROBES, 1);

    if (provider->probe_defs == NULL) {
      return Undefined();
    }

    // PROBES SECTION
    for (DTraceProbeDef *d = provider->probe_defs; d != NULL; d = d->next) {
      uint8_t argc = 0;
      dof_stridx_t argv = 0;

      for (int i = 0; d->types[i] != NULL && i < 6; i++) {
	dof_stridx_t type = strtab->Add(d->types[i]);
	argc++;
	if (argv == 0)
	  argv = type;
      }

      DTraceProbe *p = new DTraceProbe();
      p->name	    = strtab->Add(d->name);
      p->func	    = strtab->Add(d->function);
      p->noffs    = 1;
      p->enoffidx = offidx;
      p->argidx   = argidx;
      p->nenoffs  = 1;
      p->offidx   = offidx;
      p->nargc    = argc;
      p->xargc    = argc;
      p->nargv    = argv;
      p->xargv    = argv;
      p->addr     = create_probe(argc);

      argidx += argc;
      offidx++;
      
      d->probe = p;
      provider->AppendProbe(p);
      void *dof = p->Dof();
      probes->AddData(dof, sizeof(dof_probe_t));
      free(dof);
      probes->entsize = sizeof(dof_probe_t);
    }

    // PRARGS SECTION
    DOFSection *prargs = new DOFSection(DOF_SECT_PRARGS, 2);
    for (DTraceProbeDef *d = provider->probe_defs; d != NULL; d = d->next) {
      for (uint8_t i = 0; i < d->Argc(); i++) {
	prargs->AddData(&i, 1);
	prargs->entsize = 1;
      }
    }

    // estimate DOF size here, allocate
    size_t dof_size = provider->DofSize(strtab);

    DOFFile *file = new DOFFile(dof_size);
    file->AppendSection(strtab);
    file->AppendSection(probes);
    file->AppendSection(prargs);

    // PROFFS SECTION
    DOFSection *proffs = new DOFSection(DOF_SECT_PROFFS, 3);
    for (DTraceProbeDef *d = provider->probe_defs; d != NULL; d = d->next) {
      uint32_t off = d->probe->ProbeOffset(file->dof, d->Argc());
      proffs->AddData(&off, 4);
      proffs->entsize = 4;
    }
    file->AppendSection(proffs);

    // PRENOFFS SECTION
    DOFSection *prenoffs = new DOFSection(DOF_SECT_PRENOFFS, 4);
    for (DTraceProbeDef *d = provider->probe_defs; d != NULL; d = d->next) {
      uint32_t off = d->probe->IsEnabledOffset(file->dof);
      prenoffs->AddData(&off, 4);
      prenoffs->entsize = 4;
    }
    file->AppendSection(prenoffs);
    
    // PROVIDER SECTION
    DOFSection *provider_s = new DOFSection(DOF_SECT_PROVIDER, 5);
    dof_provider_t p;
    memset(&p, 0, sizeof(p));
    
    p.dofpv_strtab   = 0;
    p.dofpv_probes   = 1;
    p.dofpv_prargs   = 2;
    p.dofpv_proffs   = 3;
    p.dofpv_prenoffs = 4;
    p.dofpv_name     = pv_name;
    p.dofpv_provattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING);
    p.dofpv_modattr  = DOF_ATTR(DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_EVOLVING);
    p.dofpv_funcattr = DOF_ATTR(DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_EVOLVING);
    p.dofpv_nameattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING);
    p.dofpv_argsattr = DOF_ATTR(DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING);
    provider_s->AddData(&p, sizeof(p));
    file->AppendSection(provider_s);

    file->Generate();
    file->Load();

    provider->file = file;

    return Undefined();
  }

  Handle<Value> DTraceProvider::Fire(const Arguments& args) {
    HandleScope scope;
    void *argv[6];

    void (*func0)();
    void (*func1)(void *);
    void (*func2)(void *, void *);
    void (*func3)(void *, void *, void *);
    void (*func4)(void *, void *, void *, void *);
    void (*func5)(void *, void *, void *, void *, void *);
    void (*func6)(void *, void *, void *, void *, void *, void *);

    DTraceProvider *provider = ObjectWrap::Unwrap<DTraceProvider>(args.Holder());

    if (!args[0]->IsString()) {
      return ThrowException(Exception::Error(String::New(
        "Must give probe name as first argument")));
    }

    if (!args[1]->IsFunction()) {
      return ThrowException(Exception::Error(String::New(
        "Must give probe value callback as second argument")));
    }

    String::AsciiValue probe_name(args[0]->ToString());

    // find the probe we should be firing
    DTraceProbeDef *pd;
    for (pd = provider->probe_defs; pd != NULL; pd = pd->next) {
      if (!strcmp(pd->name, *probe_name)) {
	break;
      }
    }
    if (pd == NULL) {
      return Undefined();
    }

    // perform is-enabled check
    DTraceProbe *p = pd->probe;
    void *(*isfunc)() = (void* (*)())((uint64_t)p->addr); 
    long isenabled = (long)(*isfunc)();
    if (isenabled == 0) {
      return Undefined();
    }

    // invoke fire callback
    TryCatch try_catch;

    Local<Function> cb = Local<Function>::Cast(args[1]);
    Local<Value> probe_args = cb->Call(provider->handle_, 0, NULL);

    // exception in args callback?
    if (try_catch.HasCaught()) {
      FatalException(try_catch);
      return Undefined();
    }

    // check return
    if (!probe_args->IsArray()) {
      return Undefined();
    }

    Local<Array> a = Local<Array>::Cast(probe_args);
    
    for (int i = 0; pd->types[i] != NULL && i < 6; i++) {
      if (!strcmp("char *", pd->types[i])) {
	// char *
	String::AsciiValue str(a->Get(i)->ToString());
	argv[i] = (void *) strdup(*str); // should note which arg indexes to free
      }
      else {
	// int
	argv[i] = (void *) a->Get(i)->ToInteger()->Value();
      }
    }

    switch (pd->Argc()) {
    case 0:
      func0 = (void (*)())((uint64_t)p->addr + IS_ENABLED_FUNC_LEN); 
      (void)(*func0)();
      break;
    case 1:
      func1 = (void (*)(void *))((uint64_t)p->addr + IS_ENABLED_FUNC_LEN); 
      (void)(*func1)(argv[0]);
      break;
    case 2:
      func2 = (void (*)(void *, void *))((uint64_t)p->addr + IS_ENABLED_FUNC_LEN); 
      (void)(*func2)(argv[0], argv[1]);
      break;
    case 3:
      func3 = (void (*)(void *, void *, void *))((uint64_t)p->addr + IS_ENABLED_FUNC_LEN); 
      (void)(*func3)(argv[0], argv[1], argv[2]);
      break;
    case 4:
      func4 = (void (*)(void *, void *, void *, void *))((uint64_t)p->addr + IS_ENABLED_FUNC_LEN); 
      (void)(*func4)(argv[0], argv[1], argv[2], argv[3]);
      break;
    case 5:
      func5 = (void (*)(void *, void *, void *, void *, void *))((uint64_t)p->addr + IS_ENABLED_FUNC_LEN); 
      (void)(*func5)(argv[0], argv[1], argv[2], argv[3], argv[4]);
      break;
    case 6:
      func6 = (void (*)(void *, void *, void *, void *, void *, void *))((uint64_t)p->addr + IS_ENABLED_FUNC_LEN); 
      (void)(*func6)(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
      break;
    }

    for (int i = 0; pd->types[i] != NULL && i < 6; i++) {
      if (!strcmp("char *", pd->types[i])) {
	free(argv[i]);
      }
    }

    return Undefined();
  }

  void DTraceProvider::AppendProbe(DTraceProbe *probe) {
    if (this->probes == NULL)
      this->probes = probe;
    else {
      DTraceProbe *p;
      for (p = this->probes; (p->next != NULL); p = p->next) ;
      p->next = probe;
    }
  } 

  size_t DTraceProvider::DofSize(DOFStrtab *strtab) {
    int args = 0;
    int probes = 0;
    size_t size = 0;

    for (DTraceProbeDef *d = this->probe_defs; d != NULL; d = d->next) {
      args += d->Argc();
      probes++;
    }
    
    size_t sections[8] = {
      sizeof(dof_hdr_t),
      sizeof(dof_sec_t) * 6,
      strtab->size,
      sizeof(dof_probe_t) * probes,
      sizeof(uint8_t) * args,
      sizeof(uint32_t) * probes,
      sizeof(uint32_t) * probes,
      sizeof(dof_provider_t),
    };

    for (int i = 0; i < 8; i++) {
      size += sections[i];
      size_t i = size % 8;
      if (i > 0) {
	size += (8 - i);
      }
    }

    return size;
  }

  // --------------------------------------------------------------------
  // DOFFile

  static uint8_t dof_version(uint8_t header_version) {
    uint8_t dof_version;
    /* DOF versioning: Apple always needs version 3, but Solaris can use
       1 or 2 depending on whether is-enabled probes are needed. */
#ifdef __APPLE__
    dof_version = DOF_VERSION_3;
#else
    switch(header_version) {
    case 1:
      dof_version = DOF_VERSION_1;
      break;
    case 2:
      dof_version = DOF_VERSION_2;
      break;
    default:
      dof_version = DOF_VERSION;
    }
#endif
    return dof_version;
  } 

  void DOFFile::AppendSection(DOFSection *section) {
    if (this->sections == NULL)
      this->sections = section;
    else {
      DOFSection *s;
      for (s = this->sections; (s->next != NULL); s = s->next) ;
      s->next = section;
    }
  } 

  void DOFFile::Generate() {
    dof_hdr_t header;
    memset(&header, 0, sizeof(header));

    header.dofh_ident[DOF_ID_MAG0] = DOF_MAG_MAG0;
    header.dofh_ident[DOF_ID_MAG1] = DOF_MAG_MAG1;
    header.dofh_ident[DOF_ID_MAG2] = DOF_MAG_MAG2;
    header.dofh_ident[DOF_ID_MAG3] = DOF_MAG_MAG3;
	  
    header.dofh_ident[DOF_ID_MODEL]    = DOF_MODEL_NATIVE;
    header.dofh_ident[DOF_ID_ENCODING] = DOF_ENCODE_NATIVE;
    header.dofh_ident[DOF_ID_DIFVERS]  = DIF_VERSION;
    header.dofh_ident[DOF_ID_DIFIREG]  = DIF_DIR_NREGS;
    header.dofh_ident[DOF_ID_DIFTREG]  = DIF_DTR_NREGS;

    header.dofh_ident[DOF_ID_VERSION] = dof_version(2); /* default 2, will be 3 on OSX */
    header.dofh_hdrsize = sizeof(dof_hdr_t);
    header.dofh_secsize = sizeof(dof_sec_t);
    header.dofh_secoff = sizeof(dof_hdr_t);

    header.dofh_secnum = 6; // count of sections

    uint64_t filesz = sizeof(dof_hdr_t) + (sizeof(dof_sec_t) * header.dofh_secnum);
    uint64_t loadsz = filesz;

    for (DOFSection *section = this->sections; section != NULL; section = section->next) {
      size_t pad = 0;
      section->offset = filesz;

      if (section->align > 1) {
	size_t i = section->offset % section->align;
	if (i > 0) {
	  pad = section->align - i;
	  section->offset = (pad + section->offset);
	  section->pad = pad;
	}
      }

      filesz += section->size + pad;
      if (section->flags & 1)
	loadsz += section->size + pad;
    }

    header.dofh_loadsz = loadsz;
    header.dofh_filesz = filesz;
    memcpy(this->dof, &header, sizeof(dof_hdr_t));

    size_t offset = sizeof(dof_hdr_t);
    for (DOFSection *section = this->sections; section != NULL; section = section->next) {
      void *header = section->Header();
      (void) memcpy((this->dof + offset), header, sizeof(dof_sec_t));
      free(header);
      offset += sizeof(dof_sec_t);
    }

    for (DOFSection *section = this->sections; section != NULL; section = section->next) {
      if (section->pad > 0) {
	(void) memcpy((this->dof + offset), "\0", section->pad);
	offset += section->pad;
      }
      (void) memcpy((this->dof + offset), section->data, section->size);
      offset += section->size;
    }
  }

#ifdef __APPLE__
  static const char *helper = "/dev/dtracehelper";

  static int _loaddof(int fd, dof_helper_t *dh)
  {
    int ret;
    uint8_t buffer[sizeof(dof_ioctl_data_t) + sizeof(dof_helper_t)];
    dof_ioctl_data_t* ioctlData = (dof_ioctl_data_t*)buffer;
    user_addr_t val;

    ioctlData->dofiod_count = 1LL;
    memcpy(&ioctlData->dofiod_helpers[0], dh, sizeof(dof_helper_t));
    
    val = (user_addr_t)(unsigned long)ioctlData;
    ret = ioctl(fd, DTRACEHIOC_ADDDOF, &val);

    return ret;
  }

  static int _removedof(int fd, int gen)
  {
    return 0;
  }

#else /* Solaris */

  /* ignore Sol10 GA ... */
  static const char *helper = "/dev/dtrace/helper";

  static int _loaddof(int fd, dof_helper_t *dh)
  {
    return ioctl(fd, DTRACEHIOC_ADDDOF, dh);
  }

  static int _removedof(int fd, int gen)
  {
    return ioctl(fd, DTRACEHIOC_REMOVE, gen);
  }

#endif

  void DOFFile::Load() {
    dof_helper_t dh;
    int fd;
    dof_hdr_t *dof;
    dof = (dof_hdr_t *) this->dof;

    dh.dofhp_dof  = (uintptr_t)dof;
    dh.dofhp_addr = (uintptr_t)dof;
    (void) snprintf(dh.dofhp_mod, sizeof (dh.dofhp_mod), "module");

    fd = open(helper, O_RDWR);
    this->gen = _loaddof(fd, &dh);

    (void) close(fd);
  }

  // --------------------------------------------------------------------
  // DOFStrtab

  int DOFStrtab::Add(char *string) {
    size_t length = strlen(string);

    if (this->data == NULL) {
      this->strindex = 1;
      this->data = (char *) malloc(1);
      memcpy((void *) this->data, "\0", 1);
    }

    int i = this->strindex;
    this->strindex += (length + 1);
    this->data = (char *) realloc(this->data, this->strindex);
    (void) memcpy((void *) (this->data + i), (void *)string, length + 1);
    this->size = i + length + 1;

    return i;
  }

  // --------------------------------------------------------------------
  // DOFSection

  void DOFSection::AddData(void *data, size_t length) {
    if (this->data == NULL) {
      this->data = (char *) malloc(1);
    }
    this->data = (char *) realloc((void *)this->data, this->size + length);
    (void) memcpy(this->data + this->size, data, length);
    this->size += length;
  }

  void *DOFSection::Header() {
    dof_sec_t header;
    memset(&header, 0, sizeof(header));
    
    header.dofs_flags	= this->flags;
    header.dofs_type	= this->type;
    header.dofs_offset	= this->offset;
    header.dofs_size	= this->size;
    header.dofs_entsize = this->entsize;
    header.dofs_align	= this->align;

    void *dof = malloc(sizeof(dof_sec_t));
    memcpy(dof, &header, sizeof(dof_sec_t));
    
    return dof;
  }

  // --------------------------------------------------------------------
  // DTraceProbeDef
  
  uint8_t DTraceProbeDef::Argc() {
    uint8_t argc = 0;
    for (int i = 0; this->types[i] != NULL && i < 6; i++)
      argc++;
    return argc;
  }

  // --------------------------------------------------------------------
  // DTraceProbe

  void *DTraceProbe::Dof() {
    dof_probe_t p;
    memset(&p, 0, sizeof(p));

    p.dofpr_addr     = (uint64_t) this->addr;
    p.dofpr_func     = this->func;
    p.dofpr_name     = this->name;
    p.dofpr_nargv    = this->nargv;
    p.dofpr_xargv    = this->xargv;
    p.dofpr_argidx   = this->argidx;
    p.dofpr_offidx   = this->offidx;
    p.dofpr_nargc    = this->nargc;
    p.dofpr_xargc    = this->xargc;
    p.dofpr_noffs    = this->noffs;
    p.dofpr_enoffidx = this->enoffidx;
    p.dofpr_nenoffs  = this->nenoffs;

    void *dof = malloc(sizeof(dof_probe_t));
    memcpy(dof, &p, sizeof(dof_probe_t));

    return dof;
  }

#ifdef __APPLE__
  uint32_t DTraceProbe::ProbeOffset(char *dof, uint8_t argc) {
    return (uint32_t) ((uint64_t) this->addr - (uint64_t) dof + 18);
  }

  uint32_t DTraceProbe::IsEnabledOffset(char *dof) {
    return (uint32_t) ((uint64_t) this->addr - (uint64_t) dof + 6);
  }
#else
  uint32_t DTraceProbe::ProbeOffset(char *dof, uint8_t argc) {
    return (32 + 6 + (argc * 3));
  }

  uint32_t DTraceProbe::IsEnabledOffset(char *dof) {
    return (8);
  }
#endif
  
  extern "C" void
  init(Handle<Object> target) {
    DTraceProvider::Initialize(target);
  }
  
} // namespace node
