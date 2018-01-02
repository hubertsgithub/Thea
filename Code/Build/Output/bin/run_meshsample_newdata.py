import glob
import os
import subprocess

script = '/mnt/data/projects-hubert/Thea/Code/Build/Output/bin/run_meshsample.sh'

root_dir = '/home/hlin/projects/caffe/data/mattrans/900/mvcnn_renderings/trainset'

input_dirs = glob.glob(os.path.join(root_dir,"*"))

count = 0
for input_dir in input_dirs:
    count += 1
    if '.' in input_dir:
        continue

    shapename = input_dir.strip().split('/')[-1]

    num_samples = 75
    output_name = os.path.join(input_dir, '{}_fixed_{}.pts'.format(shapename, num_samples))
    if os.path.isfile(output_name):
        print 'Fixed file exists. Skipping {}...'.format(input_dir)
        continue


    print 'Working on {}...'.format(input_dir)

    call = ' '.join([script,
                     '-n{}'.format(num_samples),
                     '-l {}'.format(os.path.join(input_dir, '{}_fixed.labels'.format(shapename))),
                     os.path.join(input_dir, '{}_fixed.obj'.format(shapename)),
                     output_name])

    print '   Calling {}'.format(call)
    subprocess.call(call, shell=True)

    print '*** Finished: {}'.format(count)

