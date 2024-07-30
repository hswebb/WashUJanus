import sys
import socketserver

def eprint(*args, **kwargs):
  print(*args, file=sys.stderr,**kwargs)

class ServerForFRIBDAQ():
  def __init__(self, ctrl):
    HOST, PORT = "localhost", 41234

    self.server = socketserver.TCPServer((HOST, PORT), MessageHandler)
    self.server.ctrl = ctrl

    print("== Starting the FRIBDAQ service server!")
    self.server.serve_forever()

class MessageHandler(socketserver.BaseRequestHandler):
  def setup(self):
    print("== Connection established")
    self.respond("== Connection established")

  def respond(self, message):
    message += "\n"
    self.request.send(message.encode("utf-8"))

  def handle(self):
    while True:
      try:
        data = self.request.recv(1024).decode("utf-8").strip()
        ctrl = self.server.ctrl

        if not data:
          print("== Client disconnected")
          break

        if data.startswith("list") or data.startswith("help"):
          message = "== Command list:\n"
          message += "     list, help   - Show this message\n"
          message += "     begin        - Click the start DAQ button on Janus GUI\n"
          message += "     end          - Click the stop DAQ button on Janus GUI\n"
          message += "     sid   SID    - Set the source ID to SID\n"
          message += "     title TITLE  - Set the title to TITLE\n"
          message += "     run   RUNNO  - Set the run number to RUNNO\n"
          message += "     ring  RING   - Set the output RingBuffer to RING\n"
          message += "     exit         - Disconnect from the server and quit\n"
          self.respond(message)

        elif data.startswith("title"):
          title = data.replace("title ", "")
          # Setting title here
          print(title)
          self.respond(f"== Setting title to {title}")

        elif data.startswith("sid"):
          sid = data.replace("sid ", "")
          # Setting source ID here
          print(sid)
          self.respond(f"== Setting source ID to {sid}")

        elif data.startswith("run"):
          runno = data.replace("run ", "")
          # Setting run number here
          print(runno)
          self.respond(f"== Setting run number to {runno}")

        elif data.startswith("ring"):
          ring = data.replace("ring ", "")
          # Setting ringbuffer name here
          print(ring)
          self.respond(f"== Setting output RingBuffer to {ring}")

        elif data == "begin":
          if ctrl.plugged.get() == 1:
            if ctrl.b_apply['state'] != 'disabled':
              print("== Saving setting changes before starting!")
              self.respond("== Saving setting changes before starting!")
              ctrl.b_apply.invoke()

            print("== Beginning the run!")
            self.respond("== Beginning the run!")
            ctrl.bstart.invoke()
          else:
            eprint("== Janus is not connected to or disconnected from the board(s)!!!")
            self.respond("== Janus is not connected to or disconnected from the board(s)!!!")

        elif data == "end":
          if ctrl.plugged.get() == 1:
            print("== Ending the run!")
            self.respond("== Ending the run!")
            ctrl.bstop.invoke()
          else:
            eprint("== Janus is not connected to or disconnected from the board(s)!!!")
            self.respond("== Janus is not connected to or disconnected from the board(s)!!!")

        else:
          eprint(f"== Unknown command: {data}")
          self.respond(f"== Unknown command: {data}")
      except ConnectionResetError:
        break
