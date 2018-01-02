import glob
import os
import subprocess

script = './run_meshfix.sh'
script = '/mnt/data/projects-hubert/Thea/Code/Build/Output/bin/run_meshfix.sh'

root_dir = '/home/hlin/projects/ShapeNetChairs/'

input_dirs = glob.glob(root_dir+"*")

count = 0
for input_dir in input_dirs:
    count += 1
    if 'visual' in input_dir:
        continue
    elif '.' in input_dir:
        continue

    output_name = '/model.fixed.nodeldup.obj'
    if os.path.isfile(input_dir+output_name):
        print 'Fixed file exists. Skipping {}...'.format(input_dir)
        continue

    ## Manually skip troublesome shapes.
    if input_dir.strip().split('/')[-1] in ['33dc06a8e0933b1efc385a284336f217']:
        print 'This shape has too many faces. Skipping {}...'.format(input_dir)
        continue

    print 'Working on {}...'.format(input_dir)
    call = ' '.join([script, input_dir+'/model.obj', input_dir+output_name])
    print '   Calling {}'.format(call)
    subprocess.call(call, shell=True)

    print '*** Finished: {}'.format(count)

