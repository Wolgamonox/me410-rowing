from datetime import datetime
import argparse

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import serial

# Create figure for plotting
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
xs = []
ys = []

# TODO create a file to store the data with the date and time


# This function is called periodically from FuncAnimation
def animate(i, ser, xs, ys):
    # Read angle from serial
    line = ser.readline().decode().strip()

    # print(line)

    angle = 0
    try:
        angle = int(line)
    except ValueError:
        return

    # Add x and y to lists
    now = datetime.now()
    xs.append(f"{now:%S}.{f'{now:%f}'[:3]}")
    ys.append(angle)

    # print(ys)

    # Limit x and y lists to 30 items
    xs = xs[-30:]
    ys = ys[-30:]

    # Draw x and y lists
    ax.clear()
    ax.plot(xs, ys)

    # Format plot
    plt.xticks(rotation=45, ha="right")
    plt.subplots_adjust(bottom=0.30)
    plt.title("Flexion of the leg")
    plt.ylabel("Angle (degrees)")
    plt.ylim(0, 500)


def main():
    args = parse_args()
    print(args)

    ser = serial.Serial(args.rig, 115200, timeout=1)

    anim = animation.FuncAnimation(fig, animate, fargs=(ser, xs, ys), interval=100)  # noqa
    plt.show()


def parse_args():
    parser = argparse.ArgumentParser(description="Monitor the active rowers synchronizer.")
    parser.add_argument(
        "--device",
        help="The serial device port to connect to.",
    )
    parser.add_argument(
        "--rig",
        help="The serial rig port to connect to.",
    )

    args = parser.parse_args()

    return args


if __name__ == "__main__":
    main()
