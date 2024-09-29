#!/usr/bin/python
import argparse

parser = argparse.ArgumentParser(description='do keey value strategy')
parser.add_argument("-t", "--target_price", type=float, default=1.5,
                  help="target price")
parser.add_argument("-i", "--increment", type=float, default=0.1,
                  help="increment step")
args = parser.parse_args()

increment = args.increment
target_price = args.target_price
if __name__ == "__main__":
    initial_price = 1.0
    shares = 10000
    initial_value = initial_price * shares
    total_value = 0.0
    iteration = 0

    current_price = initial_price * (1 + increment)
    while current_price < target_price:
        iteration += 1
        current_value = shares * current_price

        print("Round %d: +%0.2f%%" % (iteration, increment * 100))
        print("\tcurrent shares %d" % shares)
        print("\tcurrent price %0.5f" % current_price)
        print("\tcurrent value %0.5f" % current_value)

        sell_value = current_value - initial_value
        sell_shares = int(sell_value / current_price)
        sell_shares -= sell_shares % 100
        sell_value = sell_shares * current_price
        shares -= sell_shares
        current_value = shares * current_price
        total_value += sell_value

        print("\tsell shares %d" % sell_shares)
        print("\tsell value %0.5f" % sell_value)
        print("\tleft shares %d" % shares)
        print("\tleft shares %d" % shares)

        current_price *= (1 + increment)
    
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
