#include <iostream>
#include <cstring>
#include <fstream>
#include <cscore.h>
#include <opencv2/highgui.hpp>

#include <phil/common/udp.h>
#include <phil/common/args.h>

int main(int argc, const char **argv) {
  args::ArgumentParser parser("This program records camera frames and their timestamps",
                              "This program is meant to run on TK1 during Motion Capture recording tests."
                                  "However, it is general purpose and could also be used on a laptop."
                                  "It cannot be built for the RoboRIO.");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::Positional<int> device(parser, "device_number", "0 refers to /dev/video0", args::Options::Required);
  args::Positional<int> width(parser, "width", "width in pixels", args::Options::Required);
  args::Positional<int> height(parser, "height", "height in pixels", args::Options::Required);
  args::Positional<int> frames_per_second(parser, "fps", "frames per second", args::Options::Required);
  args::Positional<std::string> encoding(parser, "encoding", "Either MJPG or YUYV", args::Options::Required);
  args::Positional<int> http_port_(parser, "http", "port for http stream", args::Options::Required);
  args::Positional<uint16_t> udp_port_(parser, "udp port", "port for start/stop msg", args::Options::Required);

  try {
    parser.ParseCLI(argc, argv);
  }
  catch (args::Help &e) {
    std::cout << parser;
    return 0;
  }
  catch (args::RequiredError &e) {
    std::cout << parser;
    return 0;
  }

  cs::UsbCamera camera{"usbcam", args::get(device)};
  const int w = args::get(width);
  const int h = args::get(height);
  const int fps = args::get(frames_per_second);
  std::string enc = args::get(encoding);

  if (enc == "MJPG") {
    camera.SetVideoMode(cs::VideoMode::kMJPEG, w, h, fps);
  }
  else if (enc == "YUYV") {
    camera.SetVideoMode(cs::VideoMode::kYUYV, w, h, fps);
  }
  else {
    camera.SetVideoMode(cs::VideoMode::kMJPEG, w, h, fps);
    std::cerr << "Invalid format [" << enc << "]. Defaulting to MJPG" << std::endl;
  }

  const uint16_t udp_port = args::get(udp_port_);
  const int http_port = args::get(http_port_);

  cs::MjpegServer mjpegServer{"httpserver", http_port};
  mjpegServer.SetSource(camera);
  cs::CvSink sink{"sink"};
  sink.SetSource(camera);

  time_t now = time(0);
  tm *ltm = localtime(&now);
  char video_filename[50];
  char timestamp_filename[50];
  strftime(video_filename, 50, "out_%m_%d_%H-%M-%S.avi", ltm);
  strftime(timestamp_filename, 50, "timestamps_%m_%d_%H-%M-%S.csv", ltm);

  cv::Mat frame;
  cv::VideoWriter video(video_filename, CV_FOURCC('M', 'J', 'P', 'G'), fps, cv::Size(w, h));

  std::ofstream time_stamps_file;
  time_stamps_file.open(timestamp_filename);

  if (!time_stamps_file.good()) {
    std::cerr << "Time stamp file failed to open: " << strerror(errno) << std::endl;
    return EXIT_FAILURE;
  }

  // wait for UDP message to start
  phil::UDPServer udp_server(udp_port);
  uint8_t message = 0;
  udp_server.Read(&message, 1);

  std::cout << "Starting recording" << std::endl;

  struct timeval timeout = {0};
  timeout.tv_usec = 10;
  timeout.tv_sec = 0;
  udp_server.SetTimeout(timeout);

  while (true) {
    uint64_t time = sink.GrabFrame(frame);

    if (time == 0) {
      std::cout << "error: " << sink.GetError() << std::endl;
      continue;
    }

    video.write(frame);
    time_stamps_file << time << std::endl;

    // check for UDP message to stop
    ssize_t bytes_received = udp_server.Read(&message, 1);
    if (bytes_received > 0) {
      break;
    }
  }

  std::cout << "Stopping recording" << std::endl;
  time_stamps_file.close();

  return EXIT_SUCCESS;
}
