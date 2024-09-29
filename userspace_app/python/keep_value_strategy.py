#!/usr/bin/python
import argparse

parser = argparse.ArgumentParser(description='do keep value strategy')
parser.add_argument("-t", "--target_gap", type=float, default=0.5,
                  help="target total increase/decrease gap")
parser.add_argument("-s", "--step", type=float, default=0.1,
                  help="increment/decrement step")
parser.add_argument("-v", "--verbose", action='store_true',
                  help='print middle steps')
parser.add_argument("-d", "--decrease", action='store_true',
                  help='calculate decrease')
args = parser.parse_args()

step = args.step
initial_price = 1.0
if args.decrease == False:
    target_price = initial_price + args.target_gap
else:
    target_price = initial_price - args.target_gap
initial_shares = 10000
initial_value = initial_price * initial_shares
def increase():
    shares = initial_shares
    total_value = 0.0
    iteration = 0

    current_price = initial_price * (1 + step)
    while current_price < target_price:
        iteration += 1
        current_value = shares * current_price
        sell_value = current_value - initial_value
        sell_shares = int(sell_value / current_price)
        sell_shares -= sell_shares % 100
        sell_value = sell_shares * current_price
        total_value += sell_value

        if args.verbose:
            print("Round %d: +%0.2f%%" % (iteration, step * 100))
            print("\tcurrent shares %d" % shares)
            print("\tcurrent price %0.5f" % current_price)
            print("\tcurrent value %0.5f" % current_value)
            print("\tsell shares %d" % sell_shares)
            print("\tsell value %0.5f(%0.5f)" % (sell_value, total_value))
            print("\tleft shares %d" % (shares - sell_shares))

        shares -= sell_shares
        current_price *= (1 + step)
    
    current_price = target_price
    current_value = shares * current_price
    print("Final Price: %0.2f(+%0.2f%%)" % (target_price, (target_price - 1.0) / 1.0))
    print("\tlast shares %d" % shares)
    print("\tlast price %0.5f" % current_price)
    print("\tlast value %0.5f" % current_value)

    total_value += current_value
    print("total iterations %d" % iteration)
    print("total value %0.5f" % total_value)
    profit = total_value - initial_value
    print("total profit %0.5f(+%0.2f%%)" % (profit, (profit / initial_value) * 100))

if __name__ == "__main__":
    if args.decrease == False:
        increase()
