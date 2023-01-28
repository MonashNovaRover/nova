#!/usr/bin/python3

import numpy as np
import torch
import time

# warmup cuda
x = np.random.rand(1, 1, 1000, 1000)
xtc = torch.Tensor(x)
kernel = torch.rand(1, 1, 10, 10)
torch.nn.functional.conv2d(xtc, kernel)

kernel = kernel.to("cuda")
xtc = xtc.to("cuda")
torch.nn.functional.conv2d(xtc, kernel)

transfer_time = 0
transfer_t = 0
init_t = 0
init_time = 0

kernel = kernel.to("cpu")
t = time.time()
with torch.no_grad():
    for i in range(0, 10):

        # setup some random numbers
        init_t = time.time()
        x = np.random.rand(1, 1, 1000, 1000)
        xtc = torch.Tensor(x, device="cpu")
        init_time += time.time() - init_t

        # run convolution on cpu
        torch.conv2d(xtc, kernel)

print("cpu convolutions took: " + str(time.time() - t - init_time))
print("init time = : " + str(init_time))
init_time = 0

kernel = kernel.to("cuda")
t = time.time()
with torch.no_grad():
    for i in range(0, 10):

        # setup some random numbers on cpu
        init_t = time.time()
        x = np.random.rand(1, 1, 1000, 1000)
        xt = torch.Tensor(x, device="cpu")
        init_time += time.time() - init_t

        # transfer the numbers to the GPU
        transfer_t = time.time()
        xtc = xt.to("cuda")
        transfer_time += time.time() - transfer_t

        # run a convolution
        torch.conv2d(xtc, kernel)

print("gpu convolutions took: " + str(time.time() - t - transfer_time \
                                      - init_time))
print("init time = : " + str(init_time))
print("transfering to gpu took: + " + str(transfer_time))
