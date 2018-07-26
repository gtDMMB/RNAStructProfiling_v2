# usage: place this file in the root directory of RNAStructProfiling.
#        run program and follow prompts
#        currently only Linux is supported
#        additional OS support can be achieved using sys.platform to detect OS
#           and chaniging commands/directory naming to that convention

import os
import subprocess

def check_outputs(output_names, args):
   temp = []
   if '-g' not in args:
      temp += [output_names[0]]
   if '-sg' not in args:
      temp += [output_names[1]]
   return temp


def update_seq(seq_dir, src_dir, args):
   os.chdir(seq_dir)
   seq_name = seq_dir[seq_dir.rfind('/')+1:]
   if not os.path.exists(seq_dir + '/input'):
      print('No input folder found for sequence: ' + seq_name)
   seq_file = seq_dir + '/input/' + seq_name + '.txt'
   if not os.path.isfile(seq_file):
      print('No sequence file found. [sequenceName].txt')
   sample_file = seq_dir + '/input/' + seq_name + '.out'
   if not os.path.isfile(sample_file):
      print('No sample file found. [sequenceName].out')
   if not os.path.exists(seq_dir + '/input') or not os.path.isfile(seq_file) or not os.path.isfile(seq_file):
      return 1
   old_path = seq_dir + '/old'
   if not os.path.exists(old_path):
      os.makedirs(old_path)
   output_names = ['/out.dot','/out_consolidated.dot']
   output_names = check_outputs(output_names, args)
   out_file_names = ['/verbose.txt','/structure.out','/stem_structure.out','/helices.txt','/key.txt'] + output_names
   for f in out_file_names + [x + '.png' for x in output_names] + ['/comparison.txt']:
      if os.path.isfile(seq_dir + f):
         os.rename(seq_dir + f, old_path + f)
   os.chdir(src_dir)
   for f in out_file_names:
      if os.path.isfile(src_dir + f):
         os.remove(src_dir + f)
   subprocess.call('./RNAprofile -o out -v ' + args + ' -sfold "' + sample_file + '" "' + seq_file + '" > verbose.txt', shell=True)
   for f in out_file_names:
      if os.path.isfile(src_dir + f):
         os.rename(src_dir + f, seq_dir + f)
      else:
         return 1
   os.chdir(seq_dir)
   for f in output_names:
      subprocess.call('dot -T png -o ' + f[1:-4] + '.png ' + f[1:] + ' > /dev/null', shell=True)
   return 0


def update_out_folder(out_folder_name, args):
   out_folder_dir = output_dir + '/' + out_folder_name
   os.chdir(out_folder_dir)
   seq_names = sorted(next(os.walk('.'))[1])
   for s in seq_names:
      if update_seq(out_folder_dir + '/' + s, src_dir, args) == 0:
         print('SUCCESS: ' + s)
      else:
         print('FAILURE: ' + s)
   print('Done: ' +  out_folder_name)
   return 0


top_dir = os.path.dirname(os.path.realpath(__file__))
src_dir = top_dir + '/src'
if not os.path.isdir(src_dir):
   print('/src folder not found')
   exit(1)
output_dir = top_dir + '/output'
if not os.path.isdir(output_dir):
   print('/output folder not found')
   exit(1)
os.chdir(output_dir)
out_folder_names = sorted(next(os.walk('.'))[1])
args = raw_input('Enter any aditional arguments to pass: ')
output_prompt = '1. All\n' + '\n'.join([str(x+2) + '. ' + (out_folder_names[x]).capitalize() for x in range(len(out_folder_names))]) + '\nSelect an output folder to update: '  
while True:
   try:
      out_folder_choice = int(input(output_prompt)) - 2
   except NameError:
      print 'Please enter a number\n'
   else:
      if -1 <= out_folder_choice < len(out_folder_names):
         break
      else:
         print 'Out of range. Try again\n'
os.chdir(src_dir)
subprocess.call('make clean>/dev/null;make>/dev/null', shell=True)
if out_folder_choice == -1:
   for name in out_folder_names:
      update_out_folder(name, args)
else:
   update_out_folder(out_folder_names[out_folder_choice], args)
exit(0)
