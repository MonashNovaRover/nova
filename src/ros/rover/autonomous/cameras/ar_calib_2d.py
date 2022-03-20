import numpy as np
import matplotlib.pyplot as plt

def main(P: np.ndarray, Q: np.ndarray):
    """
    Function to generate a least-squares best fit affine
    transformation to two sets of points. Points in p
    should correspond to points in Q
    :param P: (DIMENSIONS, n) array of transformed points
    :param Q: (DIMENSIONS, n) array of original points
    :return A, t: A is a transformation matrix and t a translation
            vector to map Q onto P with a least-squares fit
    """
    assert(P.shape == Q.shape, "Shapes don't match!")
    n, m = P.shape
    new_Q = np.ones((n + 1, m))
    new_Q[:n, :m] = Q

    Q = np.matmul(new_Q, new_Q.T)
    C = np.matmul(P, new_Q.T).T
    output = np.linalg.solve(Q, C)

    np.save("ar_2d_calibration2.npy", output)

    A = output[:n, :]
    t = output[-1, :]

    return A.T, t.T


def plot_fit(Q, P, nearly_P):
    fig = plt.figure()
    ax = fig.add_subplot(111)

    plt.xlim([-2.5, 6])
    plt.ylim([-2.5, 5])

    plt.plot(Q[0, :], Q[1, :], 'ro', label="Observed points")
    plt.plot(P[0, :], P[1, :], 'bo', label="True points")
    plt.plot(nearly_P[0, :], nearly_P[1, :], 'go', label="Approximate points")

    ax.set_aspect("equal", adjustable='box')

    plt.legend()
    plt.grid()

    plt.savefig("Affine_approx.png")
    plt.show()

# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    Q = np.load("approx2.npy").T
    print(Q.shape)
    P = np.load("true2.npy").T
    print(P.shape)
    B, s = main(P, Q)
    nearly_P = np.matmul(B, Q) + s.reshape(2, 1)
    plot_fit(Q, P, nearly_P)
# See PyCharm help at https://www.jetbrains.com/help/pycharm/
