#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <libaio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
using std::cout;
using std::printf;
int simpleRead() {
  constexpr int kNR = 100;
  constexpr int kSize = 4096;
  // init ctx
  io_context_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  int ret = io_setup(kNR, &ctx);
  if (ret != 0) {
    cout << "io_setup error\n";
    return -1;
  }
  // prepare cb
  iocb cb;
  iocb *cbs;
  char buf[kSize];
  int fd = open("./test.txt", O_RDWR | O_CREAT);
  if (fd < 0) {
    cout << "open error\n";
    return -1;
  }
  io_prep_pread(&cb, fd, buf, kSize, 0);
  cbs = &cb;
  // submit
  ret = io_submit(ctx, 1, &cbs);
  if (ret != 1) {
    cout << "io_submit error\n";
    return -1;
  }
  // get event
  io_event events[1];
  ret = io_getevents(ctx, 1, 1, events, nullptr);
  iocb *result = (struct iocb *)events[0].obj;
  printf("%s\n", (char *)result->u.c.buf);
  // destroy
  ret = io_destroy(ctx);
  if (ret != 0) {
    cout << "io_destroy error\n";
    return -1;
  }
  return 0;
}

void libaioCallback(io_context_t ctx, struct iocb *iocb, long res, long res2) {
  printf("res: %ld, res2: %ld, nbytes: %lu, offset: %lld\n", res, res2,
         iocb->u.c.nbytes, iocb->u.c.offset);
  printf("buf: %s\n\n", (char *)iocb->u.c.buf);
}

void getEvents(io_context_t ctx, const int kEvents) {
  io_event events[kEvents];
  int num = io_getevents(ctx, kEvents, kEvents, events, nullptr);
  printf("io_getevents num: %d\n", num);
  for (int i = 0; i < num; i++) {
    io_callback_t io_callback = (io_callback_t)events[i].data;
    io_callback(ctx, events[i].obj, events[i].res, events[i].res2);
  }
}

int readByKernelAio() {
  constexpr int kBufSize = 2048;
  constexpr int kEvents = 5;
  char file_path[] = "./test.txt";
  // setup ctx
  io_context_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  if (io_setup(kEvents, &ctx) != 0) {
    printf("io_setup error\n");
    return -1;
  }
  // open file
  int fd = open(file_path, O_RDONLY | O_CREAT);
  if (fd == -1) {
    printf("open file error\n");
    return -1;
  }
  // init buf
  char *buf;
  posix_memalign((void **)&buf, kBufSize, kEvents * kBufSize);
  // init iocb
  iocb cbs[kEvents];
  iocb *cb_p[kEvents];
  // submit
  for (int i = 0; i < kEvents; i++) {
    io_prep_pread(&cbs[i], fd, buf + i * kBufSize, kBufSize, i * kBufSize);
    io_set_callback(&cbs[i], libaioCallback);
    cb_p[i] = &cbs[i];
  }
  if (io_submit(ctx, kEvents, cb_p) < 0) {
    printf("io_submit error\n");
    return -1;
  }
  // get_event
  getEvents(ctx, kEvents);
  free(buf);
  io_destroy(ctx);
  close(fd);
  return 0;
}

int writeByKernelAio() {
  constexpr int kBufSize = 2048;
  constexpr int kEvents = 1;
  char file_path[] = "./test1.txt";
  // setup ctx
  io_context_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  if (io_setup(kEvents, &ctx) != 0) {
    printf("io_setup error\n");
    return -1;
  }
  // open file
  int fd = open(file_path, O_RDWR | O_CREAT);
  if (fd == -1) {
    printf("open file error\n");
    return -1;
  }
  // init buf
  char *buf;
  char tmp[] = "writeByKernelAio test\n";
  posix_memalign((void **)&buf, kBufSize, kEvents * kBufSize);
  memset(buf, 0, kBufSize);
  memcpy(buf, tmp, kBufSize);
  // init iocb
  iocb cb;
  iocb *cbs = &cb;
  // submit
  io_prep_pwrite(&cb, fd, buf, sizeof(tmp), 0);
  if (io_submit(ctx, kEvents, &cbs) < 0) {
    printf("io_submit error\n");
    return -1;
  }
  // get_event
  io_event events[1];
  io_getevents(ctx, kEvents, kEvents, events, nullptr);
  free(buf);
  io_destroy(ctx);
  close(fd);
  return 0;
}