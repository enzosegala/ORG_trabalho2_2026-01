import numpy as np

PI = np.pi
N = 1024  # <-- replace with your FFT size


def generate_signal(N, f1, f2):
    x = np.zeros(N, dtype=np.complex128)

    if f1 == 0 and f2 == 0:
        x[:] = 10000.0 + 0j

    elif f1 == 0:
        n = np.arange(N)
        x = 10000 + 0.5 * np.cos(2 * PI * f2 * n / N) + 32j

    else:
        n = np.arange(N)
        x = (
            np.cos(2 * PI * f1 * n / N)
            + 0.5 * np.cos(2 * PI * f2 * n / N)
        )

    return x


tests = [
    ("fft_output1.txt", 0, 0),
    ("fft_output2.txt", 0, 50),
    ("fft_output3.txt", 289, 50),
]

for filename, f1, f2 in tests:

    # Generate the same input as the C code
    x = generate_signal(N, f1, f2)

    # Python FFT
    fft_py = np.fft.fft(x)

    # C FFT output
    data = np.loadtxt(filename)
    fft_c = data[:, 0] + 1j * data[:, 1]

    # Compare
    max_err = np.max(np.abs(fft_c - fft_py))
    rms_err = np.sqrt(np.mean(np.abs(fft_c - fft_py) ** 2))

    print(f"\n{filename}")
    print(f"  max error : {max_err:.6e}")
    print(f"  rms error : {rms_err:.6e}")
    print(f"  allclose  : {np.allclose(fft_c, fft_py, rtol=1e-10, atol=1e-10)}")