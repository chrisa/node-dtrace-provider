import Options, Utils, sys
from os import unlink, symlink, popen
from os.path import exists, islink

srcdir = '.'
blddir = 'build'
VERSION = '0.0.2'

def set_options(ctx):
    ctx.tool_options('compiler_cxx')

def configure(ctx):
    ctx.check_tool('compiler_cxx')
    ctx.check_tool('node_addon')
    if sys.platform.startswith("sunos") or sys.platform.startswith("darwin"):
        ctx.env.append_value('CXXFLAGS', ['-D_HAVE_DTRACE'])

def build(ctx):
    if sys.platform.startswith("sunos") or sys.platform.startswith("darwin"):
        ctx.new_task_gen(
            rule = "cd ../../libusdt && make && cd -",
            shell = True
            )
        
        t = ctx.new_task_gen('cxx', 'shlib', 'node_addon')
        t.target = 'DTraceProviderBindings'
        t.source = ['dtrace_provider.cc', 'dtrace_probe.cc']
        t.includes = ['../libusdt']
        t.staticlib = 'usdt'
        t.libpath = '../../libusdt'
        
def shutdown():
    t = 'DTraceProviderBindings.node'
    if Options.commands['clean']:
       if exists(t): unlink(t)
    if Options.commands['build']:
       if exists('build/default/' + t) and not exists(t):
       	  symlink('build/default/' + t, t)
       if exists('build/Release/' + t) and not exists(t):
          symlink('build/Release/' + t, t)
