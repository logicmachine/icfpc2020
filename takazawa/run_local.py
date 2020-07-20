import subprocess, sys

def main(args):
  if len(args) < 4:
    print('Usage: python local_run.py room_creater.out attacker.out defender.out')
    print('  python local_run.py ./room_creater.out ./accel.out ./idle.out')
    return

  endpoint = 'https://icfpc2020-api.testkontur.ru/aliens/send?apiKey=b0a3d915b8d742a39897ab4dab931721'
  room_creater_exe = args[1]
  attacker_exe = args[2]
  defender_exe = args[3]

  proc = subprocess.run([room_creater_exe, endpoint], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
  room_string = proc.stdout.decode()

  attacker_key = room_string.split('\n')[0].split()[1]
  defender_key = room_string.split('\n')[1].split()[1]

  with open('log_attacker.txt', 'w') as attacker_log, open('log_defender.txt', 'w') as defender_log:
      # with open('log_attacker_stdout.txt', 'w') as attacker_log_stdout, open('log_defender_stdout.txt', 'w') as defender_log_stdout:
      print('run:', [attacker_exe, endpoint, attacker_key])
      print('run:', [defender_exe, endpoint, defender_key])
      ap = subprocess.Popen([attacker_exe, endpoint, attacker_key], stderr=attacker_log)
      dp = subprocess.Popen([defender_exe, endpoint, defender_key], stderr=defender_log)
      ap.communicate()
      dp.communicate()

  print()
  print('keys:', attacker_key, defender_key)
  print('Input this key to Galaxy GUI to see replay')

if __name__ == '__main__':
  main(sys.argv)

  