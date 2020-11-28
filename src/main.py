#!/usr/bin/env python3

# NOTE: you will need to make sure that you have opencv installed for the confidence argument to work with pyautogui
import pyautogui
import subprocess
import sys
import time
from collections import Counter, namedtuple

card_image_paths = ["../screenshots/" + card + ".png" for card in
["ace",
"two",
"three",
"four",
"five",
"six",
"seven",
"eight",
"nine",
"ten",
"jack",
"queen",
"king"]]

def pointInside(point, box):
    return point[0] >= box.left and point[0] <= box.left + box.width and point[1] >= box.top and point[1] <= box.top + box.height

def overlaps(box_a, box_b):
    return (pointInside((box_a.left, box_a.top), box_b) or
        pointInside((box_a.left + box_a.width, box_a.top), box_b) or
        pointInside((box_a.left, box_a.top + box_a.height), box_b) or
        pointInside((box_a.left + box_a.width, box_a.top + box_a.height), box_b) or
        pointInside((box_b.left, box_b.top), box_a) or
        pointInside((box_b.left + box_b.width, box_b.top), box_a) or
        pointInside((box_b.left, box_b.top + box_b.height), box_a) or
        pointInside((box_b.left + box_b.width, box_b.top + box_b.height), box_a))

def click(coords):
    pyautogui.moveTo(x=coords[0], y=coords[1], duration=pyautogui.MINIMUM_DURATION)
    pyautogui.mouseDown()
    time.sleep(0.025)
    pyautogui.mouseUp()

# Returns the card that is most likely to be in the given region, where it is, and the
# confidence level that it was found at
def mostLikely(screenshot, search_region, possibilities):
    card_images = []
    if len(possibilities) != 0:
        for card in possibilities:
            card_images.append((card, card_image_paths[card-1]))
    else:
        card_images = enumerate(card_image_paths, start=1)

    for conf_level in range(85, 0, -2):
        conf_level *= 0.01
        for num, img_path in card_images:
            search_result = pyautogui.locate(needleImage=img_path, haystackImage=scr, grayscale=True, confidence=conf_level, region=search_region)
            if search_result:
                return [num, search_result]
    return [0, search_region]

if __name__ == "__main__":
    print("Scanning screen for piles, this may take a while")

    # Calling screenshot once here is much faster than calling locateOnScreen() repeatedly
    scr = pyautogui.screenshot()

    # This just does a rough initial scan: we do some basic things to clean it up, but it
    # doesn't have to be perfect, as we'll scan the cards more closely in a bit
    card_positions = {}
    for num, img_path in enumerate(card_image_paths, start=1):
        card_positions[num] = []
        for new_pos in pyautogui.locateAll(needleImage=img_path, haystackImage=scr, grayscale=True, confidence=0.90):
            add_pos = True
            # Try to avoid adding overlapping regions
            for existing_pos in card_positions[num]:
                if overlaps(new_pos, existing_pos):
                    add_pos = False
            if num == 12: # "Q" looks a lot like the "0" in "10"
                for ten_pos in card_positions[10]:
                    if overlaps(new_pos, ten_pos):
                        add_pos = False
            if add_pos:
                card_positions[num].append(new_pos)

    # Some numbers (6, 8, 9, 10) look like themselves or other numbers upside-down, so we
    # remove the farthest-down ones
    for num in range(1, 14):
        while len(card_positions[num]) > 4:
            max_y = 0
            for pos in card_positions[num]:
                max_y = max(max_y, pos.top)
            card_positions[num] = [pos for pos in card_positions[num] if pos.top != max_y]

    # Find the coordinates of the piles
    min_x = 100000
    max_x = 0
    min_y = 100000
    max_y = 0
    for card, positions in card_positions.items():
        for pos in positions:
            min_x = min(min_x, pos.left)
            max_x = max(max_x, pos.left)
            min_y = min(min_y, pos.top)
            max_y = max(max_y, pos.top)
    diff_x = max_x - min_x # The distance between pile 0 and pile 3
    diff_x /= 3 # The average distance between each pile
    # Experimentally-derived adjustment: the "farthest right" number is a little too far
    diff_x = int(diff_x * 0.99)
    diff_y = max_y - min_y # The distance between the top card and the bottom card
    diff_y /= 12 # The average distance between each card in the pile
    diff_y = int(diff_y)

    # Note that each pile has the bottom-most card first
    piles = [[], [], [], []]
    # Here we try to read the individual cards more carefully
    for card_idx in range(13):
        for pile_idx in range(len(piles)):
            x_coord = min_x + diff_x*pile_idx
            y_coord = min_y + diff_y*card_idx

            # Experimentally-derived magic numbers that seem to work well
            left = int(x_coord - (diff_x*0.06))
            width = int(diff_x*0.24)
            top = int(y_coord - (diff_y*0.24))
            height = int(diff_y*1.02)
            Box = namedtuple("Box", ["left", "top", "width", "height"])
            search_box = Box(left, top, width, height)

            # Find the most likely candidates for each card, based on our early rough scan
            # If a card has several candidates, use the one the computer's most confident in
            # If there aren't any candidates, we'll search for every number
            possibilities = set()
            for card, positions in card_positions.items():
                for position in positions:
                    if overlaps(search_box, position):
                        possibilities.add(card)

            # # For debugging: move the cursor in a box around the card we're trying to ID
            # # top-left
            # pyautogui.moveTo(x=left, y=top, duration=pyautogui.MINIMUM_DURATION)
            # # top-right
            # pyautogui.moveTo(x=left+width, y=top, duration=pyautogui.MINIMUM_DURATION)
            # # bottom-right
            # pyautogui.moveTo(x=left+width, y=top+height, duration=pyautogui.MINIMUM_DURATION)
            # # bottom-left
            # pyautogui.moveTo(x=left, y=top+height, duration=pyautogui.MINIMUM_DURATION)
            # # top-left again
            # pyautogui.moveTo(x=left, y=top, duration=pyautogui.MINIMUM_DURATION)

            piles[pile_idx].append(mostLikely(scr, search_box, possibilities))

    for card_idx in range(13):
        for pile_idx in range(len(piles)):
            print("{:2d} ".format(piles[pile_idx][card_idx][0]), end='')
        print()

    while True:
        print("If these piles look right to you, hit enter. Otherwise, input corrections in form 'col row corrected_card'")
        cmd = input("> ")
        if len(cmd) == 0:
            break
        cmd = cmd.split()
        piles[int(cmd[0])][int(cmd[1])][0] = int(cmd[2])
        print("Corrected piles:")
        for card_idx in range(13):
            for pile_idx in range(len(piles)):
                print("{:2d} ".format(piles[pile_idx][card_idx][0]), end='')
            print()

    # Convert the piles list to a string that we can give to the C++ solver program
    in_str = ""
    for pile in piles:
        for card in pile:
            in_str += str(card[0])
            in_str += "\n"

    print("Finding optimal solution")
    commands = subprocess.run("./mf83_main", input=in_str, text=True, capture_output=True).stdout

    print("Entering solution")
    for command in commands.split():
        if command == "-":
            time.sleep(0.1) # Have to wait for the cards to move on the screen
            next_stack_pos = pyautogui.locateCenterOnScreen("../screenshots/next_stack.png", confidence=0.8)
            if next_stack_pos:
                click(next_stack_pos)
            else:
                input("Couldn't find next stack button, hit enter after you've clicked it")
        else:
            pile_idx = int(command)
            card_coords = piles[pile_idx][-1][1]
            click(card_coords)
            piles[pile_idx] = piles[pile_idx][:-1]
