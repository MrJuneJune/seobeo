#include <seobeo/os.h>
#include <seobeo/server.h>
#include <sys/event.h>

#ifdef MACOS 

static int global_initial_fd = -1;

void CleanupClient(int client_fd)
{
  // Disable Nagle's algorithm (optional for closed socket, but harmless)
  int opt = 1;
  setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

  if (global_initial_fd != -1)
  {
    struct kevent ev;
    EV_SET(&ev, client_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(global_initial_fd, &ev, 1, NULL, 0, NULL);
  }

  close(client_fd);
}

void RunEventLoop(const int server_fd)
{
  struct kevent change_event;
  struct kevent events_buffer[MAX_EVENTS];

  int kq = kqueue();
  if (kq == -1)
  {
    perror("kqueue");
    return;
  }

  global_initial_fd = kq;

  EV_SET(&change_event, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1)
  {
    perror("kevent register");
    close(kq);
    return;
  }

  while (!stop_server)
  {
    int n = kevent(kq, NULL, 0, events_buffer, MAX_EVENTS, NULL);
    if (n == -1)
    {
      if (errno == EINTR) continue;
      perror("kevent");
      break;
    }

    for (int i = 0; i < n; i++)
    {
      int fd = (int)events_buffer[i].ident;

      if (fd == server_fd)
      {
        while (1)
        {
          int client_fd = accept(server_fd, NULL, NULL);
          if (client_fd == -1)
          {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
              perror("accept");
            }
            break;
          }

          if (SetNonBlocking(client_fd) == -1)
          {
            perror("SetNonBlocking");
            close(client_fd);
            continue;
          }

          int opt = 1;
          setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

          EV_SET(&change_event, client_fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
          if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1)
          {
            perror("kevent add client");
            close(client_fd);
            continue;
          }
        }
      }
      else
      {
        Enqueue(&client_queue, fd);
      }
    }
  }

  close(kq);
  global_initial_fd = -1;
}

#endif //MACOS
