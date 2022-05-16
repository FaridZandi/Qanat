import re, sys, time

# obtianed reprinter class from here:
# https://stackoverflow.com/a/15586020
class Reprinter:
    def __init__(self):
        self.text = ''

    def moveup(self, lines):
        for _ in range(lines):
            sys.stdout.write("\x1b[A")

    def reprint(self, text):
        # Clear previous text by overwritig non-spaces with spaces
        self.moveup(self.text.count("\n"))
        sys.stdout.write(re.sub(r"[^\s]", " ", self.text))

        # Print new text
        lines = min(self.text.count("\n"), text.count("\n"))
        self.moveup(lines)
        sys.stdout.write(text)
        self.text = text

def get_events(logfile):
    m_events = {}
    event_started = False
    with open(logfile) as f:
        event_key = ""
        event_msg = ""
        for line in f:
            if any(blocked_word in line for blocked_word in ["retransmit", "flow", "fid", "MyTopo"]):
                continue
            if not event_started:
                if not line.startswith("------"):
                    continue
                else:
                    event_started = True
                    event_msg = ""
                    event_key = next(f)
                    next(f) # skip the next line
                    m_events[event_key] = event_msg 
                    continue
            else:
                if not line.strip(): # it's empty line so event finished
                    event_started = False
                    continue
                m_events[event_key] += line
    return m_events



if __name__ == "__main__":
    logfile = "./protocol.out"
    if (len(sys.argv)>1):
        logfile = sys.argv[1]
    print(logfile)
    reprinter = Reprinter()

    m_events = get_events(logfile)
    print(len(m_events))
    for event in m_events.values():
        reprinter.reprint(event)
        time.sleep(1)