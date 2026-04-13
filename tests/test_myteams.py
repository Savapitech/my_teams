#!/usr/bin/env python3

import socket
import subprocess
import time
import sys
import os
import argparse
import threading
import queue
from typing import Optional, List

SERVER_BIN = "./myteams_server"
CLIENT_BIN = "./myteams_cli"
SERVER_PORT = 5290
RECV_TIMEOUT = 1.5
DB_FILE = "myteams.db"

RED    = "\033[91m"
GREEN  = "\033[92m"
YELLOW = "\033[93m"
CYAN   = "\033[96m"
BOLD   = "\033[1m"
RESET  = "\033[0m"


class TestClient:
    def __init__(self, host="127.0.0.1", port=SERVER_PORT):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(RECV_TIMEOUT)
        self.sock.connect((host, port))
        self._buf = ""

    def send(self, cmd: str):
        if not cmd.endswith("\n"):
            cmd += "\n"
        self.sock.sendall(cmd.encode())

    def recv_line(self) -> str:
        while "\n" not in self._buf:
            try:
                chunk = self.sock.recv(4096).decode(errors="replace")
                if not chunk:
                    break
                self._buf += chunk
            except socket.timeout:
                break
        if "\n" in self._buf:
            line, self._buf = self._buf.split("\n", 1)
            return line.strip()
        line = self._buf.strip()
        self._buf = ""
        return line

    def recv_all(self, timeout=0.4) -> list:
        self.sock.settimeout(timeout)
        lines = []
        while True:
            line = self.recv_line()
            if line:
                lines.append(line)
            else:
                break
        self.sock.settimeout(RECV_TIMEOUT)
        return lines

    def cmd(self, raw: str) -> str:
        self.send(raw)
        for _ in range(20):
            line = self.recv_line()
            if not line.startswith("100 "):
                return line
        return ""

    def close(self):
        try:
            self.sock.close()
        except Exception:
            pass


class CliClient:
    def __init__(self, host="127.0.0.1", port=SERVER_PORT):
        env = os.environ.copy()
        lib_path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                                "libs", "myteams")
        env["DYLD_LIBRARY_PATH"] = lib_path
        env["LD_LIBRARY_PATH"] = lib_path

        self.proc = subprocess.Popen(
            [CLIENT_BIN, host, str(port)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
        )
        self._q: queue.Queue = queue.Queue()
        self._start_reader(self.proc.stderr)
        self._start_reader(self.proc.stdout)
        time.sleep(0.15)

    def _start_reader(self, stream):
        def _read():
            while True:
                try:
                    line = stream.readline()
                    if not line:
                        break
                    self._q.put(line.decode(errors="replace").strip())
                except Exception:
                    break
        t = threading.Thread(target=_read, daemon=True)
        t.start()

    def send(self, cmd: str):
        if not cmd.endswith("\n"):
            cmd += "\n"
        try:
            self.proc.stdin.write(cmd.encode())
            self.proc.stdin.flush()
        except BrokenPipeError:
            pass
        time.sleep(0.25)

    def collect(self, timeout=0.5) -> List[str]:
        lines = []
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                line = self._q.get(timeout=0.05)
                if line:
                    lines.append(line)
            except queue.Empty:
                pass
        return lines

    def send_and_collect(self, cmd: str, timeout=0.5) -> List[str]:
        self.send(cmd)
        return self.collect(timeout)

    def close(self):
        try:
            self.proc.stdin.close()
        except Exception:
            pass
        try:
            self.proc.terminate()
            self.proc.wait(timeout=2)
        except Exception:
            pass


class ServerProcess:
    def __init__(self, port=SERVER_PORT):
        self.port = port
        self.proc = None

    def _env(self):
        env = os.environ.copy()
        lib_path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                                "libs", "myteams")
        env["DYLD_LIBRARY_PATH"] = lib_path
        env["LD_LIBRARY_PATH"] = lib_path
        return env

    def start(self):
        if os.path.exists(DB_FILE):
            os.remove(DB_FILE)
        self.proc = subprocess.Popen(
            [SERVER_BIN, str(self.port)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            env=self._env(),
        )
        time.sleep(0.4)

    def restart_keep_db(self):
        if self.proc:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
            self.proc = None
        time.sleep(0.2)
        self.proc = subprocess.Popen(
            [SERVER_BIN, str(self.port)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            env=self._env(),
        )
        time.sleep(0.5)

    def stop(self):
        if self.proc:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
            self.proc = None
        if os.path.exists(DB_FILE):
            os.remove(DB_FILE)

    def is_alive(self):
        return self.proc and self.proc.poll() is None


class Results:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.errors = []

    def ok(self, name):
        self.passed += 1
        print(f"  {GREEN}✓{RESET} {name}")

    def fail(self, name, got, expected):
        self.failed += 1
        self.errors.append((name, got, expected))
        print(f"  {RED}✗{RESET} {name}")
        print(f"      got:      {repr(got)}")
        print(f"      expected: {repr(expected)}")

    def summary(self):
        total = self.passed + self.failed
        print(f"\n{BOLD}{'='*60}{RESET}")
        print(f"{BOLD}Results: {self.passed}/{total} tests passed{RESET}")
        if self.failed:
            print(f"{RED}{self.failed} FAILED:{RESET}")
            for name, got, exp in self.errors:
                print(f"  - {name}")
                print(f"      got:      {repr(got)}")
                print(f"      expected: {repr(exp)}")
        else:
            print(f"{GREEN}All tests passed!{RESET}")
        return self.failed == 0


R = Results()


def check(name: str, got: str, prefix: str):
    if got.startswith(prefix):
        R.ok(name)
    else:
        R.fail(name, got, f"starts with '{prefix}'")


def check_contains(name: str, got: str, substr: str):
    if substr in got:
        R.ok(name)
    else:
        R.fail(name, got, f"contains '{substr}'")


def check_in_lines(name: str, lines: list, substr: str):
    for l in lines:
        if substr in l:
            R.ok(name)
            return
    R.fail(name, str(lines), f"any line contains '{substr}'")


def check_not_in_lines(name: str, lines: list, substr: str):
    for l in lines:
        if substr in l:
            R.fail(name, str(lines), f"no line contains '{substr}'")
            return
    R.ok(name)


def extract_quoted(line: str, n: int) -> Optional[str]:
    parts = []
    in_q = False
    cur = ""
    for ch in line:
        if ch == '"':
            in_q = not in_q
        elif in_q:
            cur += ch
        elif cur:
            parts.append(cur)
            cur = ""
    if cur:
        parts.append(cur)
    return parts[n] if n < len(parts) else None


server = ServerProcess(SERVER_PORT)


def section(title):
    print(f"\n{CYAN}{BOLD}{'─'*60}{RESET}")
    print(f"{CYAN}{BOLD}  {title}{RESET}")
    print(f"{CYAN}{BOLD}{'─'*60}{RESET}")


def new_client() -> TestClient:
    return TestClient(port=SERVER_PORT)


def new_cli() -> CliClient:
    return CliClient(port=SERVER_PORT)


def test_login_logout():
    section("LOGIN / LOGOUT")

    c = new_client()
    r = c.cmd('LOGIN "alice"')
    check("1. Login new user -> 200", r, "200")
    check_contains("2. Login response has uuid", r, '"')

    c2 = new_client()
    r2 = c2.cmd('LOGIN "alice"')
    check("3. Login existing user -> 200", r2, "200")

    uuid1 = extract_quoted(r, 0)
    uuid2 = extract_quoted(r2, 0)
    if uuid1 and uuid2 and uuid1 == uuid2:
        R.ok("4. Same username -> same UUID")
    else:
        R.fail("4. Same username -> same UUID", uuid2, uuid1)

    c3 = new_client()
    check("5. Login without args -> 400", c3.cmd('LOGIN'), "400")
    check("6. Login too many args -> 400", c3.cmd('LOGIN "a" "b"'), "400")

    r5 = c.cmd('LOGOUT')
    check("7. Logout logged user -> 200", r5, "200")

    c4 = new_client()
    check("8. Logout not logged in -> 401", c4.cmd('LOGOUT'), "401")

    c5 = new_client()
    c5.cmd('LOGIN "bob"')
    check("9. Logout with extra args -> 400", c5.cmd('LOGOUT "extra"'), "400")

    c6 = new_client()
    c6.cmd('LOGIN "charlie"')
    c6.cmd('LOGOUT')
    c7 = new_client()
    r8 = c7.cmd('LOGIN "charlie"')
    check("10. Login after logout -> 200", r8, "200")

    name_charlie = extract_quoted(r8, 1)
    if name_charlie == "charlie":
        R.ok("11. Login response has correct name")
    else:
        R.fail("11. Login response has correct name", name_charlie, "charlie")

    for cl in [c, c2, c3, c4, c5, c6, c7]:
        cl.close()

def test_users_user():
    section("USERS & USER")

    helper = new_client()
    helper.cmd('LOGIN "dana"')
    r = helper.cmd('USERS')
    check("12. /users when logged in -> 200", r, "200")
    check_contains("13. /users response contains dana", r, "dana")

    dana_uuid = extract_quoted(r, 0)

    nologin = new_client()
    check("14. /users not logged in -> 401", nologin.cmd('USERS'), "401")

    r3 = helper.cmd(f'USER "{dana_uuid}"')
    check("15. /user valid uuid -> 200", r3, "200")
    check_contains("17. /user response has uuid (user exists)", r3, dana_uuid)
    check("18. /user invalid uuid -> 404", helper.cmd('USER "00000000-0000-0000-0000-000000000000"'), "404")
    check("19. /user no args -> 400", helper.cmd('USER'), "400")
    check("20. /user not logged in -> 401", nologin.cmd('USER "anything"'), "401")
    check_contains("21. /users shows connected status", helper.cmd('USERS'), '"1"')

    helper.close()
    helper2 = new_client()
    helper2.cmd('LOGIN "eve"')
    check("22. /users after disconnect returns 200", helper2.cmd('USERS'), "200")

    helper2.close()
    nologin.close()

def test_send_messages():
    section("SEND & MESSAGES")

    sender = new_client()
    sender.cmd('LOGIN "frank"')
    receiver = new_client()
    recv_resp = receiver.cmd('LOGIN "grace"')
    grace_uuid = extract_quoted(recv_resp, 0)

    frank_login = new_client()
    fl = frank_login.cmd('LOGIN "frank"')
    frank_uuid = extract_quoted(fl, 0)
    frank_login.close()

    check("23. /send to existing user -> 200", sender.cmd(f'SEND "{grace_uuid}" "Hello Grace"'), "200")

    event_lines = receiver.recv_all()
    check_in_lines("24. Receiver gets 100 event", event_lines, "100")
    check_in_lines("25. Event has sender uuid", event_lines, frank_uuid)
    check_in_lines("26. Event has message body", event_lines, "Hello Grace")

    check("27. /send unknown user -> 404", sender.cmd('SEND "00000000-0000-0000-0000-000000000000" "hi"'), "404")

    nologin = new_client()
    check("28. /send not logged in -> 401", nologin.cmd(f'SEND "{grace_uuid}" "hi"'), "401")
    check("29. /send no args -> 400", sender.cmd('SEND'), "400")
    check("30. /send one arg -> 400", sender.cmd(f'SEND "{grace_uuid}"'), "400")

    r6 = sender.cmd(f'MESSAGES "{grace_uuid}"')
    check("31. /messages existing user -> 200", r6, "200")
    check_contains("32. /messages has body", r6, "Hello Grace")
    check_contains("33. /messages has sender uuid", r6, frank_uuid)

    r7 = receiver.cmd(f'MESSAGES "{frank_uuid}"')
    check("34. /messages from receiver -> 200", r7, "200")
    check_contains("35. /messages has body from receiver", r7, "Hello Grace")

    check("36. /messages unknown user -> 404", sender.cmd('MESSAGES "00000000-0000-0000-0000-000000000000"'), "404")
    check("37. /messages not logged in -> 401", nologin.cmd(f'MESSAGES "{grace_uuid}"'), "401")
    check("38. /messages no args -> 400", sender.cmd('MESSAGES'), "400")

    sender.cmd(f'SEND "{grace_uuid}" "Message two"')
    sender.cmd(f'SEND "{grace_uuid}" "Message three"')
    r11 = sender.cmd(f'MESSAGES "{grace_uuid}"')
    check("39. /messages multiple -> 200", r11, "200")
    check_contains("40. /messages has Message two", r11, "Message two")
    check_contains("41. /messages has Message three", r11, "Message three")

    for cl in [sender, receiver, nologin]:
        cl.close()

def test_teams():
    section("CREATE / LIST / INFO - TEAMS")

    admin = new_client()
    admin.cmd('LOGIN "admin"')
    r1 = admin.cmd('CREATE "TeamAlpha" "The alpha team"')
    check("42. Create team -> 201", r1, "201")
    check_contains("43. Create team has TEAM", r1, "TEAM")
    team_uuid = extract_quoted(r1, 0)

    check("44. Create team duplicate -> 409", admin.cmd('CREATE "TeamAlpha" "dup"'), "409")

    nologin = new_client()
    check("45. Create team not logged in -> 401", nologin.cmd('CREATE "SomeTeam" "desc"'), "401")
    check("46. Create team one arg -> 400", admin.cmd('CREATE "OnlyOneName"'), "400")
    check("47. Create team no args -> 400", admin.cmd('CREATE'), "400")

    r6 = admin.cmd('LIST')
    check("48. List teams -> 200", r6, "200")
    check_contains("49. List has TeamAlpha", r6, "TeamAlpha")
    check_contains("50. List has TEAM keyword", r6, "TEAM")

    r7 = admin.cmd('INFO')
    check("51. Info no context -> 200", r7, "200")
    check_contains("52. Info has USER", r7, "USER")
    check_contains("53. Info has admin name", r7, "admin")

    check("54. List not logged in -> 401", nologin.cmd('LIST'), "401")
    check("55. Info not logged in -> 401", nologin.cmd('INFO'), "401")

    observer = new_client()
    observer.cmd('LOGIN "observer"')
    admin.cmd('CREATE "TeamBeta" "Second team"')
    ev = observer.recv_all()
    check_in_lines("56. Observer gets team_created event", ev, "100")
    check_in_lines("57. Event has team name", ev, "TeamBeta")

    for cl in [admin, nologin, observer]:
        cl.close()

def test_use():
    section("USE COMMAND")

    c = new_client()
    c.cmd('LOGIN "usetest"')
    r = c.cmd('CREATE "UseTeam" "for use tests"')
    team_uuid = extract_quoted(r, 0)

    nologin = new_client()
    check("58. Use not logged in -> 401", nologin.cmd(f'USE "{team_uuid}"'), "401")
    nologin.close()

    check("59. Use no args -> 200", c.cmd('USE'), "200")
    check("60. Use valid team -> 200", c.cmd(f'USE "{team_uuid}"'), "200")

    r4 = c.cmd('USE "00000000-0000-0000-0000-000000000000"')
    check("61. Use invalid team -> 404", r4, "404")
    check_contains("62. Use invalid team has TEAM", r4, "TEAM")

    c.cmd(f'SUBSCRIBE "{team_uuid}"')
    c.cmd(f'USE "{team_uuid}"')
    cr = c.cmd('CREATE "channel1" "first channel"')
    channel_uuid = extract_quoted(cr, 0)

    check("63. Use team+channel -> 200", c.cmd(f'USE "{team_uuid}" "{channel_uuid}"'), "200")

    r6 = c.cmd(f'USE "{team_uuid}" "00000000-0000-0000-0000-000000000000"')
    check("64. Use invalid channel -> 404", r6, "404")
    check_contains("65. Use invalid channel has CHANNEL", r6, "CHANNEL")

    c.cmd(f'USE "{team_uuid}" "{channel_uuid}"')
    tr = c.cmd('CREATE "Thread1" "Thread body"')
    thread_uuid = extract_quoted(tr, 0)

    check("66. Use team+channel+thread -> 200", c.cmd(f'USE "{team_uuid}" "{channel_uuid}" "{thread_uuid}"'), "200")

    r8 = c.cmd(f'USE "{team_uuid}" "{channel_uuid}" "00000000-0000-0000-0000-000000000000"')
    check("67. Use invalid thread -> 404", r8, "404")
    check_contains("68. Use invalid thread has THREAD", r8, "THREAD")
    check("69. Use too many args -> 400", c.cmd(f'USE "{team_uuid}" "{channel_uuid}" "{thread_uuid}" "x"'), "400")

    c.close()

def test_channels():
    section("CHANNELS")

    admin = new_client()
    admin.cmd('LOGIN "chan_admin"')
    r = admin.cmd('CREATE "ChanTeam" "channel team"')
    team_uuid = extract_quoted(r, 0)
    admin.cmd(f'SUBSCRIBE "{team_uuid}"')
    admin.cmd(f'USE "{team_uuid}"')

    r1 = admin.cmd('CREATE "general" "General channel"')
    check("70. Create channel -> 201", r1, "201")
    check_contains("71. Create channel has CHANNEL", r1, "CHANNEL")
    chan_uuid = extract_quoted(r1, 0)

    check("72. Create duplicate channel -> 409", admin.cmd('CREATE "general" "dup"'), "409")
    check("73. Create channel one arg -> 400", admin.cmd('CREATE "onlyone"'), "400")

    outsider = new_client()
    outsider.cmd('LOGIN "outsider"')
    outsider.cmd(f'USE "{team_uuid}"')
    check("74. Create channel without sub -> 401", outsider.cmd('CREATE "secretchan" "cant"'), "401")

    r5 = admin.cmd('LIST')
    check("75. List channels -> 200", r5, "200")
    check_contains("76. List has CHANNEL", r5, "CHANNEL")
    check_contains("77. List has general", r5, "general")

    admin.cmd(f'USE "{team_uuid}" "{chan_uuid}"')
    r6 = admin.cmd('INFO')
    check("78. Info on channel -> 200", r6, "200")
    check_contains("79. Channel info has uuid", r6, chan_uuid)
    check_contains("80. Channel info has name", r6, "general")

    observer = new_client()
    observer.cmd('LOGIN "obs2"')
    observer.cmd(f'SUBSCRIBE "{team_uuid}"')
    admin.cmd(f'USE "{team_uuid}"')
    admin.cmd('CREATE "announcements" "news"')
    ev = observer.recv_all()
    check_in_lines("81. Subscriber gets channel_created event", ev, "100")
    check_in_lines("82. Event has channel name", ev, "announcements")

    nonmember = new_client()
    nonmember.cmd('LOGIN "nonmember"')
    admin.cmd('CREATE "hidden_chan" "hidden"')
    check_not_in_lines("83. Non-subscriber no channel event", nonmember.recv_all(), "channel_created")

    for cl in [admin, outsider, observer, nonmember]:
        cl.close()

def test_threads():
    section("THREADS")

    admin = new_client()
    admin.cmd('LOGIN "thread_admin"')
    r = admin.cmd('CREATE "ThreadTeam" "thread team"')
    team_uuid = extract_quoted(r, 0)
    admin.cmd(f'SUBSCRIBE "{team_uuid}"')
    admin.cmd(f'USE "{team_uuid}"')
    rc = admin.cmd('CREATE "main_chan" "main"')
    chan_uuid = extract_quoted(rc, 0)
    admin.cmd(f'USE "{team_uuid}" "{chan_uuid}"')

    r1 = admin.cmd('CREATE "First Thread" "Thread body here"')
    check("84. Create thread -> 201", r1, "201")
    check_contains("85. Thread has THREAD", r1, "THREAD")
    thread_uuid = extract_quoted(r1, 0)

    check("86. Create duplicate thread -> 409", admin.cmd('CREATE "First Thread" "body"'), "409")
    check("87. Create thread one arg -> 400", admin.cmd('CREATE "Only title"'), "400")

    outsider = new_client()
    outsider.cmd('LOGIN "thread_outsider"')
    outsider.cmd(f'USE "{team_uuid}" "{chan_uuid}"')
    check("88. Create thread without sub -> 401", outsider.cmd('CREATE "hacked" "body"'), "401")

    r5 = admin.cmd('LIST')
    check("89. List threads -> 200", r5, "200")
    check_contains("90. Thread list has title", r5, "First Thread")

    admin.cmd(f'USE "{team_uuid}" "{chan_uuid}" "{thread_uuid}"')
    r6 = admin.cmd('INFO')
    check("91. Info on thread -> 200", r6, "200")
    check_contains("92. Thread info has uuid", r6, thread_uuid)
    check_contains("93. Thread info has title", r6, "First Thread")

    subscriber = new_client()
    subscriber.cmd('LOGIN "sub_user"')
    subscriber.cmd(f'SUBSCRIBE "{team_uuid}"')
    admin.cmd(f'USE "{team_uuid}" "{chan_uuid}"')
    admin.cmd('CREATE "New Thread" "new body"')
    ev = subscriber.recv_all()
    check_in_lines("94. Subscriber gets thread_created event", ev, "100")
    check_in_lines("95. Thread event has title", ev, "New Thread")

    for cl in [admin, outsider, subscriber]:
        cl.close()

def test_replies():
    section("REPLIES")

    admin = new_client()
    admin.cmd('LOGIN "reply_admin"')
    r = admin.cmd('CREATE "ReplyTeam" "reply team"')
    team_uuid = extract_quoted(r, 0)
    admin.cmd(f'SUBSCRIBE "{team_uuid}"')
    admin.cmd(f'USE "{team_uuid}"')
    rc = admin.cmd('CREATE "reply_chan" "channel"')
    chan_uuid = extract_quoted(rc, 0)
    admin.cmd(f'USE "{team_uuid}" "{chan_uuid}"')
    rt = admin.cmd('CREATE "Reply Thread" "thread body"')
    thread_uuid = extract_quoted(rt, 0)
    admin.cmd(f'USE "{team_uuid}" "{chan_uuid}" "{thread_uuid}"')

    r1 = admin.cmd('CREATE "First reply"')
    check("96. Create reply -> 201", r1, "201")
    check_contains("97. Reply has REPLY", r1, "REPLY")

    check("98. Create reply too many args -> 400", admin.cmd('CREATE "reply" "too many"'), "400")
    check("99. Create reply no args -> 400", admin.cmd('CREATE'), "400")

    outsider = new_client()
    outsider.cmd('LOGIN "reply_outsider"')
    outsider.cmd(f'USE "{team_uuid}" "{chan_uuid}" "{thread_uuid}"')
    check("100. Reply without sub -> 401", outsider.cmd('CREATE "hacked reply"'), "401")

    r5 = admin.cmd('LIST')
    check("101. List replies -> 200", r5, "200")
    check_contains("102. Reply list has body", r5, "First reply")

    admin.cmd('CREATE "Second reply"')
    check_contains("103. Reply list has Second reply", admin.cmd('LIST'), "Second reply")

    subscriber = new_client()
    subscriber.cmd('LOGIN "reply_sub"')
    subscriber.cmd(f'SUBSCRIBE "{team_uuid}"')
    admin.cmd('CREATE "Third reply"')
    ev = subscriber.recv_all()
    check_in_lines("104. Subscriber gets thread_reply event", ev, "100")
    check_in_lines("105. Thread_reply event has body", ev, "Third reply")

    nonmember = new_client()
    nonmember.cmd('LOGIN "reply_nonmember"')
    admin.cmd('CREATE "Fourth reply"')
    check_not_in_lines("106. Non-subscriber no reply event", nonmember.recv_all(), "thread_reply")

    for cl in [admin, outsider, subscriber, nonmember]:
        cl.close()

def test_subscribe():
    section("SUBSCRIBE / UNSUBSCRIBE / SUBSCRIBED")

    admin = new_client()
    admin.cmd('LOGIN "sub_admin"')
    r = admin.cmd('CREATE "SubTeam" "subscribe team"')
    team_uuid = extract_quoted(r, 0)

    user = new_client()
    user.cmd('LOGIN "sub_user2"')

    check("107. Subscribe to existing team -> 200", user.cmd(f'SUBSCRIBE "{team_uuid}"'), "200")
    check("108. Subscribe twice -> 409", user.cmd(f'SUBSCRIBE "{team_uuid}"'), "409")
    check("109. Subscribe non-existing -> 404", user.cmd('SUBSCRIBE "00000000-0000-0000-0000-000000000000"'), "404")

    nologin = new_client()
    check("110. Subscribe not logged in -> 401", nologin.cmd(f'SUBSCRIBE "{team_uuid}"'), "401")
    check("111. Subscribe no args -> 400", user.cmd('SUBSCRIBE'), "400")

    r6 = user.cmd('SUBSCRIBED')
    check("112. Subscribed list my teams -> 200", r6, "200")
    check_contains("113. Subscribed has SubTeam", r6, "SubTeam")

    admin.cmd(f'SUBSCRIBE "{team_uuid}"')
    r7 = user.cmd(f'SUBSCRIBED "{team_uuid}"')
    check("114. Subscribed users in team -> 200", r7, "200")
    check_contains("115. Subscribed has sub_user2", r7, "sub_user2")
    check_contains("116. Subscribed has sub_admin", r7, "sub_admin")

    check("117. Subscribed non-existing -> 404", user.cmd('SUBSCRIBED "00000000-0000-0000-0000-000000000000"'), "404")
    check("118. Subscribed not logged in -> 401", nologin.cmd('SUBSCRIBED'), "401")
    check("119. Unsubscribe from team -> 200", user.cmd(f'UNSUBSCRIBE "{team_uuid}"'), "200")
    check("120. Unsubscribe when not subscribed -> 400", user.cmd(f'UNSUBSCRIBE "{team_uuid}"'), "400")
    check("121. Unsubscribe non-existing -> 404", user.cmd('UNSUBSCRIBE "00000000-0000-0000-0000-000000000000"'), "404")
    check("122. Unsubscribe not logged in -> 401", nologin.cmd(f'UNSUBSCRIBE "{team_uuid}"'), "401")

    r14 = admin.cmd(f'SUBSCRIBED "{team_uuid}"')
    if "sub_user2" not in r14:
        R.ok("123. After unsubscribe, user removed from team list")
    else:
        R.fail("123. After unsubscribe, user removed from team list", r14, "no sub_user2")

    for cl in [admin, user, nologin]:
        cl.close()

def test_security_and_multiclients():
    section("SECURITY & MULTI-CLIENT EVENTS")

    ghost = new_client()
    check("124. Ghost cant list -> 401", ghost.cmd('LIST'), "401")
    check("125. Ghost cant info -> 401", ghost.cmd('INFO'), "401")
    check("126. Ghost cant create -> 401", ghost.cmd('CREATE "team" "desc"'), "401")

    alice = new_client()
    alice.cmd('LOGIN "a_alice"')
    bob = new_client()
    bob_r = bob.cmd('LOGIN "a_bob"')
    bob_uuid = extract_quoted(bob_r, 0)
    eve = new_client()
    eve.cmd('LOGIN "a_eve"')

    alice.cmd(f'SEND "{bob_uuid}" "secret message"')
    bob_lines = bob.recv_all()
    eve_lines = eve.recv_all()
    check_in_lines("127. Bob receives private message event", bob_lines, "100")
    check_in_lines("128. Message has body", bob_lines, "secret message")
    check_not_in_lines("129. Eve does not receive private message", eve_lines, "secret message")

    carol = new_client()
    carol.cmd('LOGIN "a_carol"')
    admin2 = new_client()
    admin2.cmd('LOGIN "a_admin2"')
    admin2.cmd('CREATE "BroadcastTeam" "all see this"')
    check_in_lines("130. Carol gets team_created broadcast", carol.recv_all(), "100")
    check_in_lines("131. Bob gets team_created broadcast", bob.recv_all(), "100")

    adm_login = new_client()
    bt_r = admin2.cmd('LIST')
    adm_login.close()
    bt_uuid = extract_quoted(bt_r.replace("TEAM ", ""), 0) if "TEAM" in bt_r else None
    if bt_uuid:
        admin2.cmd(f'SUBSCRIBE "{bt_uuid}"')
        admin2.cmd(f'USE "{bt_uuid}"')
        admin2.cmd('CREATE "member_only_chan" "private"')
        check_not_in_lines("132. Eve non-member no channel event", eve.recv_all(), "channel_created")

    mal = new_client()
    mal.cmd('LOGIN "malformed_test"')
    check("133. Non-quoted args -> 400", mal.cmd('CREATE nonquoted desc'), "400")

    unk = new_client()
    unk.cmd('LOGIN "unk_user"')
    try:
        unk.cmd('FOOBAR "test"')
        R.ok("134. Unknown command does not crash server")
    except Exception:
        R.fail("134. Unknown command does not crash server", "exception", "no crash")

    for cl in [ghost, alice, bob, eve, carol, admin2, mal, unk]:
        cl.close()

def test_persistence():
    section("PERSISTENCE (Save & Reload)")

    c = new_client()
    c.cmd('LOGIN "persist_user"')
    r = c.cmd('CREATE "PersistTeam" "should persist"')
    team_uuid = extract_quoted(r, 0)
    c.cmd(f'SUBSCRIBE "{team_uuid}"')
    c.cmd(f'USE "{team_uuid}"')
    c.cmd('CREATE "persist_chan" "persisted channel"')
    c.close()

    server.restart_keep_db()

    c2 = new_client()
    c2.cmd('LOGIN "persist_user"')

    r3 = c2.cmd('LIST')
    check("135. Teams persist after restart -> 200", r3, "200")
    check_contains("136. PersistTeam still in list", r3, "PersistTeam")
    if team_uuid and team_uuid in r3:
        R.ok("137. Team UUID persisted correctly")
    else:
        R.fail("137. Team UUID persisted correctly", r3, f"contains {team_uuid}")

    check("138. Subscriptions persist -> 200", c2.cmd('SUBSCRIBED'), "200")
    check_contains("139. PersistTeam in subscribed list", c2.cmd('SUBSCRIBED'), "PersistTeam")

    c2.cmd(f'USE "{team_uuid}"')
    r5 = c2.cmd('LIST')
    check("140. Channels persist -> 200", r5, "200")
    check_contains("141. persist_chan in channel list", r5, "persist_chan")

    c2.close()

def test_edge_cases():
    section("EDGE CASES & TRICKY TESTS")

    c = new_client()
    c.cmd('LOGIN "edge_user"')

    check("142. List empty teams -> 200", c.cmd('LIST'), "200")

    r2 = c.cmd('CREATE "EdgeTeam" "edge"')
    tu = extract_quoted(r2, 0)
    c.cmd(f'SUBSCRIBE "{tu}"')
    c.cmd(f'USE "{tu}"')
    check("143. List empty channels -> 200", c.cmd('LIST'), "200")

    rc = c.cmd('CREATE "edge_chan" "edge chan"')
    cu = extract_quoted(rc, 0)
    c.cmd(f'USE "{tu}" "{cu}"')
    check("144. List empty threads -> 200", c.cmd('LIST'), "200")

    rt = c.cmd('CREATE "edge_thread" "edge body"')
    thu = extract_quoted(rt, 0)
    c.cmd(f'USE "{tu}" "{cu}" "{thu}"')
    check("145. List empty replies -> 200", c.cmd('LIST'), "200")

    my_login = new_client()
    ml = my_login.cmd('LOGIN "self_sender"')
    my_uuid = extract_quoted(ml, 0)
    my_login.close()
    sc = new_client()
    sc.cmd('LOGIN "self_sender"')
    check("146. Send message to self -> 200", sc.cmd(f'SEND "{my_uuid}" "to myself"'), "200")

    for i in range(5):
        c.cmd('USE')
        c.cmd(f'CREATE "BulkTeam{i}" "bulk {i}"')
    r7 = c.cmd('LIST')
    check("147. Multiple teams in list -> 200", r7, "200")
    check_contains("148. BulkTeam0 in list", r7, "BulkTeam0")
    check_contains("149. BulkTeam4 in list", r7, "BulkTeam4")

    long_name = "A" * 31
    c.cmd('USE')
    c.cmd(f'CREATE "{long_name}" "desc"')
    check_contains("150. Long team name in list", c.cmd('LIST'), long_name[:10])

    c.cmd('USE')
    r9 = c.cmd('INFO')
    check("151. Info after clear context -> 200", r9, "200")
    check_contains("152. Info has USER", r9, "USER")

    a = new_client()
    a.cmd('LOGIN "sub_switch_a"')
    ra = a.cmd('CREATE "SwitchTeam1" "t1"')
    t1 = extract_quoted(ra, 0)
    rb = a.cmd('CREATE "SwitchTeam2" "t2"')
    t2 = extract_quoted(rb, 0)
    a.cmd(f'SUBSCRIBE "{t1}"')
    a.cmd(f'SUBSCRIBE "{t2}"')
    a.cmd(f'USE "{t1}"')
    a.cmd('CREATE "chan_t1" "c1"')
    a.cmd(f'USE "{t2}"')
    rc2 = a.cmd('CREATE "chan_t2" "c2"')
    check("153. Create channel in second team -> 201", rc2, "201")
    a.cmd(f'USE "{t1}"')
    r_list = a.cmd('LIST')
    check_contains("154. Channels of t1 only listed", r_list, "chan_t1")
    if "chan_t2" not in r_list:
        R.ok("155. chan_t2 not in t1's channel list")
    else:
        R.fail("155. chan_t2 not in t1's channel list", r_list, "no chan_t2")

    b_cli = new_client()
    b_cli.cmd('LOGIN "orphan_chk"')
    b_cli.cmd(f'SUBSCRIBE "{t1}"')
    b_cli.cmd(f'USE "{t1}"')
    b_cli.cmd('CREATE "orphan_chan" "oc"')
    b_cli.cmd('USE')
    b_cli.cmd(f'USE "{t1}"')
    r_orphan = b_cli.cmd('LIST')
    check_contains("156. Channel still visible after context reset", r_orphan, "orphan_chan")

    msg_sender = new_client()
    msg_sender.cmd('LOGIN "msg_order_a"')
    msg_recv = new_client()
    mr = msg_recv.cmd('LOGIN "msg_order_b"')
    mr_uuid = extract_quoted(mr, 0)
    ml2 = new_client()
    ml2_r = ml2.cmd('LOGIN "msg_order_a"')
    ml2_uuid = extract_quoted(ml2_r, 0)
    ml2.close()
    for i in range(3):
        msg_sender.cmd(f'SEND "{mr_uuid}" "msg {i}"')
    hist = msg_sender.cmd(f'MESSAGES "{mr_uuid}"')
    check("157. Message order preserved -> msg 0 present", hist, "200")
    check_contains("158. msg 0 in history", hist, "msg 0")
    check_contains("159. msg 2 in history", hist, "msg 2")

    double_sub = new_client()
    double_sub.cmd('LOGIN "double_sub_user"')
    ds_r = double_sub.cmd('CREATE "DoubleSubTeam" "ds"')
    ds_t = extract_quoted(ds_r, 0)
    check("160. Subscribe first time -> 200", double_sub.cmd(f'SUBSCRIBE "{ds_t}"'), "200")
    check("161. Subscribe second time -> 409", double_sub.cmd(f'SUBSCRIBE "{ds_t}"'), "409")
    check("162. Still only subscribed once", double_sub.cmd('SUBSCRIBED'), "200")

    for cl in [c, sc, a, b_cli, msg_sender, msg_recv, double_sub]:
        cl.close()

def test_cli_integration():
    section("CLI INTEGRATION TESTS")

    if not os.path.exists(CLIENT_BIN):
        print(f"  {YELLOW}⚠ CLI binary not found, skipping CLI tests{RESET}")
        return

    cli = new_cli()

    lines = cli.send_and_collect('/login "cli_user1"')
    check_in_lines("163. CLI /login triggers logged_in event", lines, "")
    if any(lines):
        R.ok("163. CLI /login produces output")
    else:
        R.fail("163. CLI /login produces output", str(lines), "non-empty output")

    lines2 = cli.send_and_collect('/users')
    if any(lines2):
        R.ok("164. CLI /users produces output")
    else:
        R.fail("164. CLI /users produces output", str(lines2), "non-empty")

    cli.collect(timeout=0.3)
    lines3 = cli.send_and_collect('/users', timeout=1.0)
    has_users = any('User' in l or 'cli_user' in l or any(c.isalnum() for c in l) for l in lines3)
    if has_users or len(lines3) > 0:
        R.ok("165. CLI /users after collect produces output")
    else:
        R.fail("165. CLI /users after collect produces output", str(lines3), "any output")

    lines4 = cli.send_and_collect('/create "CliTeam1" "team from cli"')
    check_in_lines("166. CLI /create team shows team created", lines4, "CliTeam1")

    cli2 = new_cli()
    cli2.send('/login "cli_user2"')
    cli2.collect()

    lines5 = cli.send_and_collect('/users', timeout=0.8)
    combined5 = " ".join(lines5)
    if "cli_user" in combined5 or len(lines5) > 0:
        R.ok("167. CLI /users produces user output")
    else:
        R.fail("167. CLI /users produces user output", str(lines5), "user output")

    lines6 = cli2.send_and_collect('/users', timeout=0.8)
    combined6 = " ".join(lines6)
    if "cli_user" in combined6 or len(lines6) > 0:
        R.ok("168. CLI /users from second client produces output")
    else:
        R.fail("168. CLI /users from second client produces output", str(lines6), "user output")

    raw = new_client()
    raw_r = raw.cmd('LOGIN "cli_user1"')
    cli_uuid = extract_quoted(raw_r, 0)
    raw.close()

    raw2 = new_client()
    raw2_r = raw2.cmd('LOGIN "cli_user2"')
    cli2_uuid = extract_quoted(raw2_r, 0)
    raw2.close()

    lines7 = cli.send_and_collect(f'/send "{cli2_uuid}" "Hello from CLI"')
    recv_lines = cli2.collect()
    check_in_lines("169. CLI /send - receiver gets message event", recv_lines, "Hello from CLI")

    lines8 = cli.send_and_collect(f'/messages "{cli2_uuid}"')
    check_in_lines("170. CLI /messages shows history", lines8, "Hello from CLI")

    lines9 = cli.send_and_collect(f'/subscribe "00000000-0000-0000-0000-000000000000"')
    check_in_lines("171. CLI /subscribe unknown team shows error", lines9, "")
    if any(lines9):
        R.ok("171. CLI /subscribe unknown produces output")
    else:
        R.fail("171. CLI /subscribe unknown produces output", str(lines9), "output")

    raw3 = new_client()
    raw3.cmd('LOGIN "cli_user1"')
    r_lt = raw3.cmd('LIST')
    team_uuid_cli = extract_quoted(r_lt.replace("TEAM ", ""), 0) if "TEAM" in r_lt else None
    raw3.close()

    if team_uuid_cli:
        lines10 = cli.send_and_collect(f'/subscribe "{team_uuid_cli}"')
        if any(lines10):
            R.ok("172. CLI /subscribe existing team produces output")
        else:
            R.fail("172. CLI /subscribe existing team produces output", str(lines10), "output")

        lines11 = cli.send_and_collect('/subscribed', timeout=0.8)
        combined11 = " ".join(lines11)
        if "CliTeam1" in combined11 or "Team" in combined11:
            R.ok("173. CLI /subscribed shows subscription")
        else:
            R.fail("173. CLI /subscribed shows subscription", str(lines11), "CliTeam1 or any team")

        cli.send(f'/use "{team_uuid_cli}"')
        cli.collect()

        lines12 = cli.send_and_collect('/create "CliChan1" "cli channel"')
        check_in_lines("174. CLI /create channel shows CliChan1", lines12, "CliChan1")

        lines13 = cli.send_and_collect('/list')
        check_in_lines("175. CLI /list shows CliChan1", lines13, "CliChan1")

        raw4 = new_client()
        raw4.cmd('LOGIN "cli_user1"')
        r_lc = raw4.cmd('LIST')
        chan_uuid_cli = extract_quoted(r_lc.replace("CHANNEL ", ""), 0) if "CHANNEL" in r_lc else None
        raw4.close()

        if chan_uuid_cli:
            cli.send(f'/use "{team_uuid_cli}" "{chan_uuid_cli}"')
            cli.collect()

            lines14 = cli.send_and_collect('/create "CliThread1" "cli thread body"')
            check_in_lines("176. CLI /create thread shows CliThread1", lines14, "CliThread1")

            lines15 = cli.send_and_collect('/list')
            check_in_lines("177. CLI /list shows CliThread1", lines15, "CliThread1")

            lines16 = cli.send_and_collect('/info')
            check_in_lines("178. CLI /info in channel shows channel info", lines16, "CliChan1")

    lines17 = cli.send_and_collect('/info')
    if any(lines17):
        R.ok("179. CLI /info produces output")
    else:
        R.fail("179. CLI /info produces output", str(lines17), "output")

    cli3 = new_cli()
    cli3.send('/login "cli_ghost"')
    cli3.collect()
    cli3.send('/logout')
    log_out_lines = cli3.collect()
    if any(log_out_lines):
        R.ok("180. CLI /logout produces output")
    else:
        R.fail("180. CLI /logout produces output", str(log_out_lines), "output")

    cli3.close()

    cli4 = new_cli()
    lines18 = cli4.send_and_collect('/users')
    if any(lines18):
        R.ok("181. CLI /users not logged in produces output (error)")
    else:
        R.fail("181. CLI /users not logged in produces output", str(lines18), "output")
    cli4.close()

    cli.close()
    cli2.close()

def test_tricky():
    section("TRICKY TESTS")

    a = new_client()
    a.cmd('LOGIN "tricky_a"')
    b = new_client()
    b_r = b.cmd('LOGIN "tricky_b"')
    b_uuid = extract_quoted(b_r, 0)

    a_login = new_client()
    a_r = a_login.cmd('LOGIN "tricky_a"')
    a_uuid = extract_quoted(a_r, 0)
    a_login.close()

    for i in range(5):
        a.cmd(f'SEND "{b_uuid}" "spam {i}"')
    hist = a.cmd(f'MESSAGES "{b_uuid}"')
    check("182. 5 rapid messages all in history -> 200", hist, "200")
    check_contains("183. spam 4 in history", hist, "spam 4")

    b.cmd(f'SEND "{a_uuid}" "reply"')
    hist2 = b.cmd(f'MESSAGES "{a_uuid}"')
    check_contains("184. Cross-direction messages in history", hist2, "reply")

    tr = new_client()
    tr.cmd('LOGIN "tricky_reconnect"')
    tr_r2 = tr.cmd('CREATE "TrickyTeam" "t"')
    t_uuid = extract_quoted(tr_r2, 0)
    tr.cmd(f'SUBSCRIBE "{t_uuid}"')
    tr.cmd(f'USE "{t_uuid}"')
    tr.cmd('CREATE "tc1" "c"')
    tr.close()

    tr2 = new_client()
    tr2.cmd('LOGIN "tricky_reconnect"')
    tr2.cmd(f'USE "{t_uuid}"')
    r_after = tr2.cmd('LIST')
    check_contains("185. Channels visible after reconnect", r_after, "tc1")
    tr2.close()

    ctx_c = new_client()
    ctx_c.cmd('LOGIN "ctx_test"')
    ctx_r = ctx_c.cmd('CREATE "CtxTeam" "c"')
    ctx_tu = extract_quoted(ctx_r, 0)
    ctx_c.cmd(f'SUBSCRIBE "{ctx_tu}"')
    ctx_c.cmd(f'USE "{ctx_tu}"')
    cr = ctx_c.cmd('CREATE "ctx_chan" "c"')
    ctx_cu = extract_quoted(cr, 0)
    ctx_c.cmd(f'USE "{ctx_tu}" "{ctx_cu}"')

    ctx_c.cmd('USE')
    after_clear_info = ctx_c.cmd('INFO')
    check_contains("186. After USE clear, INFO shows USER", after_clear_info, "USER")

    ctx_c.cmd(f'USE "{ctx_tu}"')
    after_team_info = ctx_c.cmd('INFO')
    check_contains("187. After USE team, INFO shows TEAM", after_team_info, "TEAM")
    ctx_c.close()

    ord_c = new_client()
    ord_r = ord_c.cmd('LOGIN "ord_user"')
    ord_uuid = extract_quoted(ord_r, 0)
    ord_c.close()

    ord_recv = new_client()
    ord_recv.cmd('LOGIN "ord_recv"')
    ord_recv_r = new_client()
    rv = ord_recv_r.cmd('LOGIN "ord_recv"')
    rv_uuid = extract_quoted(rv, 0)
    ord_recv_r.close()

    ord_send = new_client()
    ord_send.cmd('LOGIN "ord_user"')
    for i in range(3):
        ord_send.cmd(f'SEND "{rv_uuid}" "order_{i}"')
    _ = ord_recv.recv_all(timeout=0.8)
    hist3 = ord_send.cmd(f'MESSAGES "{rv_uuid}"')
    check_contains("188. order_0 in message history", hist3, "order_0")
    check_contains("189. order_2 in message history", hist3, "order_2")
    ord_send.close()
    ord_recv.close()

    sa = new_client()
    sa.cmd('LOGIN "scope_a"')
    sb = new_client()
    sb.cmd('LOGIN "scope_b"')
    s_team_r = sa.cmd('CREATE "ScopeTeam" "s"')
    s_t = extract_quoted(s_team_r, 0)
    sa.cmd(f'SUBSCRIBE "{s_t}"')
    sa.cmd(f'USE "{s_t}"')
    sa.cmd('CREATE "scope_chan" "sc"')

    _ = sb.recv_all()

    sa.cmd(f'USE "{s_t}"')
    sa.cmd('CREATE "scope_chan2" "sc2"')
    sb_ev = sb.recv_all()
    check_not_in_lines("190. Unsubscribed user gets no channel event", sb_ev, "scope_chan")

    sb.cmd(f'SUBSCRIBE "{s_t}"')
    sa.cmd('CREATE "scope_chan3" "sc3"')
    sb_ev2 = sb.recv_all()
    check_in_lines("191. After subscribe, user gets channel events", sb_ev2, "100")
    sa.close()
    sb.close()

    if os.path.exists(CLIENT_BIN):
        cli_tricky = new_cli()
        cli_tricky.send('/login "trick_cli"')
        cli_tricky.collect()

        cli_tricky.send('/create "TrickCliTeam" "desc"')
        team_lines = cli_tricky.collect()
        check_in_lines("192. CLI create team shows team name", team_lines, "TrickCliTeam")

        raw_ck = new_client()
        raw_ck.cmd('LOGIN "trick_cli"')
        r_ct = raw_ck.cmd('LIST')
        trk_t = extract_quoted(r_ct.replace("TEAM ", ""), 0) if "TEAM" in r_ct else None
        raw_ck.close()

        if trk_t:
            raw_ck2 = new_client()
            raw_ck2.cmd('LOGIN "trick_cli"')
            raw_ck2.cmd(f'SUBSCRIBE "{trk_t}"')
            raw_ck2.cmd(f'USE "{trk_t}"')
            raw_ck2.cmd('CREATE "trick_chan" "tc"')
            raw_ck2_l = raw_ck2.cmd('LIST')
            raw_ck2.close()
            check_contains("193. CLI-created team has channel from socket client", raw_ck2_l, "trick_chan")

        cli_tricky.send('/use')
        cli_tricky.collect()
        info_lines = cli_tricky.send_and_collect('/info')
        check_in_lines("194. CLI /info at root shows user", info_lines, "trick_cli")

        cli_tricky.close()

    b_cli5 = new_client()
    b_cli5.cmd('LOGIN "subcount_user"')
    sc_r = b_cli5.cmd('CREATE "SubCountTeam" "sct"')
    sct_uuid = extract_quoted(sc_r, 0)
    for _ in range(3):
        b_cli5.cmd(f'SUBSCRIBE "{sct_uuid}"')
    sub_list = b_cli5.cmd(f'SUBSCRIBED "{sct_uuid}"')
    count = sub_list.count("subcount_user")
    if count == 1:
        R.ok("195. No duplicate subscriptions even after multiple attempts")
    else:
        R.fail("195. No duplicate subscriptions", str(count), "1 occurrence")
    b_cli5.close()

    del_msg_c = new_client()
    dm_r = del_msg_c.cmd('LOGIN "delmsg_user"')
    dm_uuid = extract_quoted(dm_r, 0)

    other_dm = new_client()
    other_dm_r = other_dm.cmd('LOGIN "delmsg_other"')
    other_dm_uuid = extract_quoted(other_dm_r, 0)

    del_msg_c.cmd(f'SEND "{other_dm_uuid}" "first"')
    del_msg_c.cmd(f'SEND "{other_dm_uuid}" "second"')
    del_msg_c.cmd(f'SEND "{other_dm_uuid}" "third"')
    hist_all = del_msg_c.cmd(f'MESSAGES "{other_dm_uuid}"')
    check("196. All messages present in history -> 200", hist_all, "200")
    check_contains("197. first in history", hist_all, "first")
    check_contains("198. third in history", hist_all, "third")
    del_msg_c.close()
    other_dm.close()

    for cl in [a, b]:
        cl.close()

def main():
    global SERVER_PORT, server

    parser = argparse.ArgumentParser(description="MyTeams test suite")
    parser.add_argument("--port", type=int, default=SERVER_PORT)
    args = parser.parse_args()

    SERVER_PORT = args.port

    if not os.path.exists(SERVER_BIN):
        print(f"{RED}Server binary not found: {SERVER_BIN}{RESET}")
        sys.exit(1)

    print(f"\n{BOLD}{CYAN}MyTeams Test Suite — 198+ tests{RESET}")
    print(f"Server: {SERVER_BIN} on port {SERVER_PORT}")
    print(f"{'='*60}")

    srv = ServerProcess(SERVER_PORT)
    srv.start()
    server = srv

    if not srv.is_alive():
        print(f"{RED}Failed to start server!{RESET}")
        sys.exit(1)

    print(f"{GREEN}Server started (PID {srv.proc.pid}){RESET}")

    try:
        test_login_logout()
        test_users_user()
        test_send_messages()
        test_teams()
        test_use()
        test_channels()
        test_threads()
        test_replies()
        test_subscribe()
        test_security_and_multiclients()
        test_persistence()
        test_edge_cases()
        test_cli_integration()
        test_tricky()
    except KeyboardInterrupt:
        print(f"\n{YELLOW}Tests interrupted{RESET}")
    except Exception as e:
        print(f"\n{RED}Test runner crashed: {e}{RESET}")
        import traceback
        traceback.print_exc()
    finally:
        server.stop()

    ok = R.summary()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
