import sys
import serial
import pygame


screen_width = 600
screen_height = 600

matrix_width = 12
matrix_height = 12


pygame.init()
screen = pygame.display.set_mode((screen_width, screen_height))

def parse(line):
    global matrix_width, matrix_height

    # LED INFO COMMAND
    if (line.startswith(b'LEDINFO')):
        if len(line) < 11:
            print('wrong info command (expected length 12 but got {0})'.format(len(line)))
            return
        
        matrix_width = int(line[7:9], 16);
        matrix_height = int(line[9:11], 16);
        
        print('--- matrix width: {0} ---'.format(matrix_width))
        print('--- matrix height: {0} ---'.format(matrix_height))

    # LED DATA COMMAND
    elif (line.startswith(b'LEDDATA')):
        if (matrix_width == 0 or matrix_height == 0):
            print('--- matrix size must be set before data command ---')
            return
        
        if len(line) < 7 + matrix_width * matrix_height * 6:
            print('--- wrong data command (expected length {0} but got {1}) ---'.format(8 + matrix_width * matrix_height * 6, len(line)))
            return
        
        parseLedData(line[7:])
        
    # OTHER COMMAND
    else:
        try:
            print(line.decode('ascii'), end='')
        except UnicodeDecodeError:
            print('--- could not decode serial input ---')


def parseLedData(data):
    for y in range(matrix_height):
        for x in range(matrix_width):
            i = (x + y * matrix_width) * 6
            red = int(data[i:i+2], 16)
            green = int(data[i+2:i+4], 16)
            blue = int(data[i+4:i+6], 16)

            pygame.draw.rect(screen, (red, green, blue), (
                x * screen_width / matrix_width,
                y * screen_height / matrix_height,
                screen_width / matrix_width,
                screen_height / matrix_height
            ))
    
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            ser.close()
            pygame.quit()

    pygame.display.update()


if __name__ == '__main__':
    print('Virtual Matrix. Developed by Amon Benson.\n')

    ser = serial.Serial('/dev/ttyUSB0', 115200)

    while True:
        line = ser.readline()
        parse(line)
