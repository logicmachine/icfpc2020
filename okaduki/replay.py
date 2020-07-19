import subprocess
import shutil
from time import sleep

min_r = -13
max_r = 13

def send(proc, s):
  s = s.rstrip('\n')
  proc.stdin.write((s + '\n').encode('UTF-8'))
  proc.stdin.flush()
  proc.stdout.flush()
  res = proc.stdout.readline().decode('UTF-8').rstrip('\n').lstrip('> ')
  proc.stdout.flush()
  return res

def getflag(s):
  i = 0
  while not s[i].isdigit():
    i += 1
  return s[i]

def getstate(s):
  i = 0
  while not s[i].isdigit():
    i += 1
  while s[i].isdigit():
    i += 1
  while not s[i].isdigit():
    i += 1
  r = i
  while s[r].isdigit():
    r += 1
  return int(s[i:r])

def get_pnm_size(f):
  fmt = f.readline()
  sz = f.readline()
  w, h = [int(x) for x in sz.decode('ascii').rstrip('\n').split(' ')]
  return (w, h)


def main():
  loop_num = 0
  cmds = []
  nxt_cmd = 'ap ap ap interact galaxy :state ap ap cons 0 0'
  with open('input.txt') as f:
    n = int(f.readline())
    for i in range(n):
      cmds.append(f.readline())
    loop_num = int(f.readline())

  proc = subprocess.Popen(
    ["../interpreter/a.out", "galaxy.txt"],
    stdout=subprocess.PIPE,
    stdin=subprocess.PIPE
  )

  for cmd in cmds:
    res = send(proc, cmd)

  sleep(0.1)
  with open('output.pnm', 'rb') as f:
    _, battle_h = get_pnm_size(f)

  # make battle
  default_output_img = 'output.pnm'
  work_dir = './work/'
  cnt = 0
  filename = work_dir+'output_{:03}.png'.format(cnt)
  subprocess.run(['convert', default_output_img, filename])
  work_files = [filename]

  for i in range(loop_num):
    cnt += 1
    res = send(proc, nxt_cmd)

    filename = work_dir+'output_{:03}.png'.format(cnt)
    subprocess.run(['convert', default_output_img, filename])
    work_files.append(filename)

    sleep(0.1)
    with open('output.pnm', 'rb') as f:
      _, h = get_pnm_size(f)
    if h != battle_h:
      break

  proc.stdin.close()
  proc.stdout.close()
  proc.wait()

  # make gif
  subprocess.run([
    'convert', '-layers', 'optimize', '-loop', '0', '-delay', '40']
    + work_files + ['game.gif'])


if __name__ == '__main__':
  main()
