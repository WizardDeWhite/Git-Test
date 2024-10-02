#!/usr/bin/python
import argparse

parser = argparse.ArgumentParser(description='do keep value strategy')
parser.add_argument("-p", "--initial_price", type=float, default=1.0,
                  help="initial price")
parser.add_argument("-t", "--target_price", type=float, default=1.5,
                  help="target total increase/decrease gap")
parser.add_argument("-n", "--number_shares", type=int, default=1000000,
                  help="number of shares")
parser.add_argument("-s", "--step", type=float, default=0.1,
                  help="increment/decrement step")
parser.add_argument("-v", "--verbose", action='store_true',
                  help='print middle steps')
parser.add_argument("-d", "--decrease", action='store_true',
                  help='calculate decrease')
args = parser.parse_args()

target_price = args.target_price
if args.decrease == False:
    step = args.step
else:
    step = -args.step

def increase(initial_price, target_price, shares, step):
    total_value = 0.0
    iteration = 0

    if (step <= 0):
        print("ERROR: Increase with negative step is not reasonable")
        return

    if (target_price < initial_price):
        print("ERROR: Increase with smaller target price is not reasonable")
        return

    initial_value = initial_price * shares
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
    print("Final Price: %0.2f(%0.2f+%0.2f%%) with step +%0.2f%%" %
            (target_price, initial_price, (target_price - initial_price) * 100, step * 100))
    print("\tlast shares %d" % shares)
    print("\tlast price %0.5f" % current_price)
    print("\tlast value %0.5f" % current_value)

    total_value += current_value
    print("total iterations %d" % iteration)
    print("total value %0.5f" % total_value)
    profit = total_value - initial_value
    print("total profit %0.5f(+%0.2f%%)" % (profit, (profit / initial_value) * 100))
    return shares

def decrease(initial_price, target_price, shares, step):
    total_buy = 0.0
    iteration = 0

    if (step >= 0):
        print("ERROR: Decrease with positive step is not reasonable")
        return

    if (target_price > initial_price):
        print("ERROR: Decrease with greater target price is not reasonable")
        return

    initial_value = initial_price * shares
    current_price = initial_price * (1 + step)
    while current_price > target_price:
        iteration += 1
        current_value = shares * current_price
        buy_value = initial_value - current_value
        buy_shares = int(buy_value / current_price)
        buy_shares -= buy_shares % 100
        buy_value = buy_shares * current_price
        total_buy += buy_value

        if args.verbose:
            print("Round %d: %0.2f%%" % (iteration, step * 100))
            print("\tcurrent shares %d" % shares)
            print("\tcurrent price %0.5f" % current_price)
            print("\tcurrent value %0.5f" % current_value)
            print("\tbuy shares %d" % buy_shares)
            print("\tbuy value %0.5f(%0.5f)" % (buy_value, total_buy))
            print("\tvalue after buy %0.5f" % (current_value + buy_value))
            print("\tleft shares %d" % (shares + buy_shares))

        shares += buy_shares
        current_price *= (1 + step)

    current_price = target_price
    current_value = shares * current_price
    buy_value = initial_value - current_value
    buy_shares = int(buy_value / current_price)
    buy_shares -= buy_shares % 100
    buy_value = buy_shares * current_price
    total_buy += buy_value
    print("Final Price: %0.2f(%0.2f%0.2f%%) with step %0.2f%%" %
            (target_price, initial_price, (target_price - initial_price ) * 100, step * 100))
    print("\tcurrent shares %d" % shares)
    print("\tcurrent price %0.5f" % current_price)
    print("\tcurrent value %0.5f" % current_value)
    print("\tlast buy shares %d" % buy_shares)
    print("\tlast buy value %0.5f(%0.5f)" % (buy_value, total_buy))
    print("\tfinal value %0.5f" % (current_value + buy_value))
    print("\tfinal shares %d" % (shares + buy_shares))

    print("total iterations %d" % iteration)
    print("total buy %0.5f(+%0.2f%%)" % (total_buy, (total_buy / initial_value) * 100))
    return shares + buy_shares

if __name__ == "__main__":
    if args.decrease == False:
        increase(args.initial_price, target_price, args.number_shares, step)
    else:
        decrease(args.initial_price, target_price, args.number_shares, step)
