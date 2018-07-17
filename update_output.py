import os
import glob

def update_seq(seq_dir, top_dir):
    

top_dir = os.path.dirname(os.path.realpath(__file__))
output_dir = top_dir + '/output'
os.chdir(output_dir)
out_folder_names = sorted(next(os.walk('.'))[1])

update_seq(output_dir + '/development/Agrobacterium_tumefa.1_sfold', top_dir)

output_prompt = '\n'.join([str(x+1) + '. ' + (out_folder_names[x]).capitalize() for x in range(len(out_folder_names))]) + '\nSelect an output folder to update: '  
while True:
   try:
       out_folder_choice = int(input(output_prompt)) - 1
   except NameError:
       print 'Please enter a number\n'
   else:
       if 0 <= out_folder_choice < len(out_folder_names):
           break
       else:
           print 'Out of range. Try again\n'
out_folder_dir = output_dir + '/' + out_folder_names[out_folder_choice]
