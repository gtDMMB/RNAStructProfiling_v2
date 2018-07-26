import os
import shutil

def create_seq_file(prof_file_name, out_file_name):
    out_file = open(out_file_name, 'w')
    with open(prof_file_name, 'r') as fp:
        for i, line in enumerate(fp):
            if i == 0:
                line = line[(line.find(' is ') + 4):line.find(' with ')]            
                out_file.write(line)


def complete_seq_class(class_name, seq_class_dir, out_class_dir):
    if not os.path.isdir(out_class_dir):
        os.makedirs(out_class_dir)
    prof_dir = seq_class_dir + '/Profiles'
    sfold_dir = seq_class_dir + '/Sfold_output'
    seq_names = [name[:-4] for name in sorted(os.listdir(prof_dir))]
    seq_dict = {}
    for name in seq_names:
        input_dir = out_class_dir + '/' + name + '/input'
        seq_file_name = prof_dir + '/' + name + '.out'
        out_seq_file_name = input_dir + '/' + name +'.txt'
        sfold_file_name = sfold_dir + '/' + name + '.out'
        out_sfold_file_name = input_dir + '/' + name + '.out'
        if not os.path.isfile(seq_file_name):
            print('Missing verbose output file: ' + name + '.out in class: ' + class_name)
            continue
        if not os.path.isfile(sfold_file_name):
            print('Missing sfold sample file: ' + name + '.out in class: ' + class_name)
            continue
        if not os.path.isdir(input_dir):
            os.makedirs(input_dir)
        create_seq_file(seq_file_name, out_seq_file_name)
        shutil.copyfile(sfold_file_name, out_sfold_file_name)
    
    

top_dir = os.path.dirname(os.path.realpath(__file__))
out_dir = top_dir + '/output'
os.chdir(top_dir)
seq_class_names = sorted(next(os.walk('.'))[1])
if 'output' in seq_class_names:
    seq_class_names.remove('output')
seq_class_dirs = sorted([top_dir + '/' + name for name in seq_class_names])
out_class_dirs = sorted([out_dir + '/' + name for name in seq_class_names])
if not os.path.isdir(out_dir):
    os.makedirs(out_dir)
for i in range(len(seq_class_names)):
    complete_seq_class(seq_class_names[i], seq_class_dirs[i], out_class_dirs[i])
    
    
    
