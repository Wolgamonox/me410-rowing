from datetime import datetime
import argparse

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import matplotlib.animation as animation
import serial

# Create figure for plotting
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
xs = []
ys_device = []
ys_rig = []
# TODO create a file to store the data with the date and time

def rescale(value, min, max, new_range=[0,1]):
    rescaled = (value-min)/(max-min)
    return rescaled * (new_range[1] - new_range[0])

# This function is called periodically from FuncAnimation
def animate(i, ser_device, ser_rig, xs, ys_device, ys_rig):
    # Read angle from serial

    print("Device:", float(ser_device.readline().decode()))
    print("Rig:", float(ser_rig.readline().decode()))


    angle_device = 0
    angle_rig = 0
    try:
        angle_device = float(ser_device.readline().decode())
        angle_device = angle_device * 100

        angle_rig = float(ser_rig.readline().decode())
        angle_rig = rescale(angle_rig, 0.209, 0.417, [0,100]) # TODO Don´t hard code the min and max follower encoder values
    except ValueError:
        return

    # Add x and y to lists
    now = datetime.now()
    xs.append(f"{now:%S}.{f'{now:%f}'[:3]}")
    ys_device.append(angle_device)
    ys_rig.append(angle_rig)

    # print(ys)

    # Limit x and y lists to 30 items
    xs = xs[-30:]
    ys_device = ys_device[-30:]
    ys_rig = ys_rig[-30:]

    # Draw x and y lists
    ax.clear()
    ax.plot(xs, ys_device, color='blue', label='Leader')
    ax.plot(xs, ys_rig, color='red', label='Follower')
    ax.legend(loc='lower left')

    # Format plot
    plt.xticks(rotation=45, ha="right")
    plt.gca().xaxis.set_major_locator(ticker.MultipleLocator(4))  # Set ticks every 4 units
    plt.subplots_adjust(bottom=0.30)
    plt.title("Flexion of the leg")
    plt.ylabel("Angle (degrees)")
    plt.ylim(-3, 105)


def main():
    args = parse_args()
    print(args)

    ser_device = serial.Serial(args.device, 115200, timeout=1)
    ser_rig = serial.Serial(args.rig, 115200, timeout=1)
    anim = animation.FuncAnimation(fig, animate, fargs=(ser_device, ser_rig, xs, ys_device, ys_rig), interval=10, save_count=100)  # noqa
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
