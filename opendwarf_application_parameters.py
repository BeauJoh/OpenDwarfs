
#Benchmark parameters:
kmeans = {'name':'kmeans',
          'alias':'kmeans',
          'default':'-i ../test/mapreduce/kmeans/65536_34f.txt',
          'tiny':'-g -f 30 -p 256',     #< 32K, object increments of 1KiB
          'small':'-g -f 30 -p 2048',   #< 256K, object increments of 8KiB
          'medium':'-g -f 30 -p 65600', #< 8196K, object increments of 256KiB
          'large':'-g -f 30 -p 131072', #> 8196K, object increments of 2048KiB
          'full name':'K-Means Clustering'}
kmeans_coarse_iteration_profile = {
        'name':'kmeans_profiling_outer_loop',
        'alias':'kmeans',
        'default':'-i ../test/mapreduce/kmeans/65536_34f.txt',
        'tiny':'-g -f 30 -p 256',     # 1KiB
        'small':'-g -f 30 -p 2048',   # 8KiB
        'medium':'-g -f 30 -p 65600', # 256KiB
        'large':'-g -f 30 -p 131072', # 2048KiB
        'full name':'K-Means Clustering'}
lud = {'name':'lud',
       'alias':'lud',
       'default':'-i ../test/dense-linear-algebra/lud/3072.dat',
       'tiny':'-s 80',#25KiB < 32KiB
       'small':'-s 240',#225KiB < 256KiB
       'medium':'-s 1440',#8100KiB < 8196KiB
       'large':'-s 4096', #65536KiB > 8196KiB
       'full name':'Lower Upper Decomposition'}
lud_coarse_iteration_profile = {
       'name':'lud_profiling_outer_loop',
       'alias':'lud',
       'default':'-i ../test/dense-linear-algebra/lud/3072.dat',
       'tiny':'-s 80',#25KiB < 32KiB
       'small':'-s 240',#225KiB < 256KiB
       'medium':'-s 1440',#8100KiB < 8196KiB
       'large':'-s 4096', #65536KiB > 8196KiB
       'full name':'Lower Upper Decomposition'}
csr = {'name':'csr',
       'alias':'csr',
       'default':'-i ../test/sparse-linear-algebra/SPMV/csr_65536.txt',
       'tiny':'-i ../test/sparse-linear-algebra/SPMV/tiny',#generated with ./createcsr -n 736 -d 5000, all matrices have 0.5% density (99.5% sparse) 31.6KiB < 32KiB
       'small':'-i ../test/sparse-linear-algebra/SPMV/small',#2416, 254.8KiB
       'medium':'-i ../test/sparse-linear-algebra/SPMV/medium',#14336, 8195.5KiB
       'large':'-i ../test/sparse-linear-algebra/SPMV/large',#16384, 10677.8KiB
       'full name':'Compressed Sparse Row'}
fft = {'name':'openclfft',
       'alias':'openclfft',
       'default':'128',
       'tiny':'2048', #32KiB
       'small':'16384 ',#256KiB
       'medium':'524288', #8192KiB
       'large':'2097152', #32768KiB
       'full name':'Fast Fourier Transform'}
dwt = {'name':'dwt2d',
       'alias':'dwt2d',
       'default':'-l 3 ../test/spectral-methods/dwt2d/airplane.ppm -w airplane', #writes out the dwt2d wavelet coefficients in a visual form to pgm format
       'tiny':'-l 3 ../test/spectral-methods/dwt2d/72x54-gum.ppm tiny-gum-coefficients', #32KiB
       'small':'-l 3 ../test/spectral-methods/dwt2d/200x150-gum.ppm small-gum-coefficients',#256KiB
       'medium':'-l 3 ../test/spectral-methods/dwt2d/1152x864-gum.ppm medium-gum-coefficients', #8192KiB
       'large':'-l 3 ../test/spectral-methods/dwt2d/3648x2736-gum.ppm large-gum-coefficients', #32768KiB
       'full name':'2D Discrete Wavelet Transform'}
gem = {'name':'gemnoui',
       'alias':'gem',
       'default':'../test/n-body-methods/gem/nucleosome 80 1 0',
       'tiny':'../test/n-body-methods/gem/4TUT 80 1 0', #31.3KiB
       'small':'../test/n-body-methods/gem/2D3V 80 1 0', #252.0KiB
       'medium':'../test/n-body-methods/gem/nucleosome 80 1 0', #7498.1KiB
       'large':'../test/n-body-methods/gem/1KX5 80 1 0', #10970.2KiB
       'full name':'Gemnoui'}
srad = {'name':'srad',
        'alias':'srad',
        'default':'256 256 0 127 0 127 0.5 2',
        'tiny':'80 16 0 127 0 127 0.5 1',#30KiB
        'small':'128 80 0 127 0 127 0.5 1',#240KiB
        'medium':'1024 336 0 127 0 127 0.5 1',#8064KiB
        'large':'2048 1024 0 127 0 127 0.5 1',#49152KiB
        'full name':'Speckle Reducing Anisotropic Diffusion'}
cfd = {'name':'cfd',
       'alias':'cfd',
       'default':'../test/unstructured-grids/cfd/fvcorr.domn.097K',
       'tiny':'../test/unstructured-grids/cfd/128.dat',#23.066KiB
       'small':'../test/unstructured-grids/cfd/1284.dat',#253.066KiB
       'medium':'../test/unstructured-grids/cfd/45056.dat',#8096KiB
       'large':'../test/unstructured-grids/cfd/193474.dat',#34776.066KiB
       'full name':'Computational Fluid Dynamics'}
crc = {'name':'crc',
       'alias':'crc',
       'default':'-i ../test/combinational-logic/crc/crc_1000x8192.txt',
       'tiny':'-i ../test/combinational-logic/crc/crc_1000x2000.txt',#31.31KiB
       'small':'-i ../test/combinational-logic/crc/crc_1000x16000.txt',#250.06KiB
       'medium':'-i ../test/combinational-logic/crc/crc_1000x524000.txt',#8187.56KiB
       'large':'-i ../test/combinational-logic/crc/crc_1000x4194304.txt',#65536.06KiB
       'full name':'Cyclic-Redundancy Check'}
bfs = {'name':'bfs',
       'alias':'bfs',
       'default':'../test/graph-traversal/bfs/graph65536.txt',
       'tiny':'../test/graph-traversal/bfs/graph650.txt',#30.40KiB
       'small':'../test/graph-traversal/bfs/graph5376.txt',#252.03KiB
       'medium':'../test/graph-traversal/bfs/graph172032.txt',#8061.18KiB
       'large':'../test/graph-traversal/bfs/graph1048576.txt',#49147.97KiB
       'full name':'Breadth-First Search'}
nw = {'name':'needle',
      'alias':'needle',
      'default':"2048 10",
      'tiny':"48 10",#18.76KiB (must be a multiple of 16)
      'small':"176 10",#244.76KiB 
      'medium':'1008 10',#7953.76KiB
      'large':'4096 10',#131136KiB
      'full name':'Needleman-Wunsch'}
swat = {'name':'swat',
        'alias':'swat',
        'default':'../test/dynamic-programming/swat/query1K1 ../test/dynamic-programming/swat/sampledb1K1',
        'full name':'Smith-Waterman'}
nqueens = {'name':'nqueens',
           'alias':'nqueens',
           'default':'20',
           'full name':'N-Queens'}
hmm = {'name':'bwa_hmm',
       'alias':'bwa_hmm',
       'default':'-n 30 -v n',
       'full name':'Baum-Welch Algorithm, Hidden Markov Model'}
tdm = {'name':'tdm',
       'alias':'tdm',
       'default':'../test/finite-state-machine/tdm/sim-64-size200.csv ../test/finite-state-machine/tdm/ivl.txt ../test/finite-state-machine/tdm/30-episodes.txt 128',
       'full name':'Temporal Data Mining'}

#Dwarfs as clusters of Benchmarks:
dense_linear_algebra = [kmeans,lud]
sparse_linear_algebra = [csr]
spectral_methods = [fft,dwt]
n_body_methods = [gem]
structured_grid_methods = [srad]
unstructured_grid_methods = [cfd]
combinational_logic = [crc]
graph_traversal = [bfs]
dynamic_programming = [nw,swat]
backtrack_branch_and_bound = [nqueens]
graphical_models = [hmm]
finite_state_machines = [tdm]

dwarfs = [dense_linear_algebra,
          sparse_linear_algebra,
          spectral_methods,
          n_body_methods,
          structured_grid_methods]


