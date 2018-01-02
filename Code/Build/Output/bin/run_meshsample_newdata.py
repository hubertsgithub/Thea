import glob
import os
import subprocess

script = '/mnt/data/projects-hubert/Thea/Code/Build/Output/bin/run_meshsample.sh'
root_dir = '/home/hlin/projects/caffe/data/mattrans/900/mvcnn_renderings/trainset'
list_of_shapes = '/home/hlin/projects/caffe/data/mattrans/900/mvcnn_renderings/sorted_meshes_according_to_percentage_group_labeled.txt'

if os.path.exists(list_of_shapes):
    print 'Reading input shapes from {}...'.format(list_of_shapes)
    input_dirs = []
    threshold = 0.5  # Only keep shapes which are at least <threshold> labeled.
    fp = open(list_of_shapes, 'r')
    for line in fp:
        splits = line.strip().split(' ')

        if float(splits[1]) >= threshold:
            splits[0] = splits[0].split('/')[1]
            input_dir = os.path.join(root_dir, splits[0])
            assert os.path.exists(input_dir)
            input_dirs.append(input_dir)

    fp.close()
else:
    print 'Reading directory ({}) for input shapes...'.format(root_dir)
    input_dirs = glob.glob(os.path.join(root_dir,"*"))


num_shapes = len(input_dirs)
print 'Total number of shapes: {}'.format(num_shapes)

## 90% of shapes will be used for train/val. These all need to fit into memory.
# Total available memory possible: 256G. 2D data requires ~16G. Need to leave some memory for other overhead. So assume can get 225G.
# 225G / 3 = 75G (need to stack)
# Points per shape to sample: (75G * 2615 pts/G) / (0.9*num_shapes shapes)
num_samples = int(75 * 2615 / (0.9 * num_shapes))

## Manually set number of samples
num_samples = 250

print 'Sampling {} points per shape.'.format(num_samples)
raw_input('Press any key to begin...')
print '-------------------------------------'

count = 0
for input_dir in input_dirs:
    count += 1
    if '.' in input_dir:
        continue

    shapename = input_dir.strip().split('/')[-1]

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

