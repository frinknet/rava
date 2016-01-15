#define RAVA_CORE           "rava.core"
#define RAVA_CORE_STREAM    "rava.core.stream"

#define RAVA_PROCESS        "rava.process"
#define RAVA_PROCESS_COND   "rava.process.cond"
#define RAVA_PROCESS_FIBER  "rava.process.fiber"
#define RAVA_PROCESS_IDLE   "rava.process.idle"
#define RAVA_PROCESS_THREAD "rava.process.thread"
#define RAVA_PROCESS_SPAWN  "rava.process.spawn"
#define RAVA_PROCESS_TIMER  "rava.process.timer"

#define RAVA_SOCKET         "rava.socket"
#define RAVA_SOCKET_PIPE    "rava.socket.pipe"
#define RAVA_SOCKET_TCP     "rava.socket.tcp"
#define RAVA_SOCKET_UDP     "rava.socket.udp"

#define RAVA_SYSTEM         "rava.system"
#define RAVA_SYSTEM_FS      "rava.system.fs"
#define RAVA_SYSTEM_FILE    "rava.system.fs.file"

/* state flags */
#define RAVA_STATE_START (1 << 0)
#define RAVA_STATE_READY (1 << 1)
#define RAVA_STATE_MAIN  (1 << 2)
#define RAVA_STATE_WAIT  (1 << 3)
#define RAVA_STATE_JOIN  (1 << 4)
#define RAVA_STATE_DEAD  (1 << 5)
