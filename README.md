# Regame - Cloud Gaming Engine

[简体中文](README.zh-CN.md) | English

`Regame - Cloud Gaming Engine` is a set of technologies developed by Edge computing team of [Ksyun](https://www.ksyun.com/) to serve the cloud game scene.

Cloud gaming is a method of playing video games that is done using remote hardware. You don't have to download or install games locally, just begin playing the game immediately because it is being run on remote servers. 

Cloud gaming engine is a core technology to host ordinary games on remote server and start cloud gaming service.

`Regame` is still under development. It would be great if you could help.

[Download Regame v0.3 here](https://ks3-cn-beijing.ksyun.com/liuguang/regame_v0.3.zip)

Demo videos (Chinese):

- [01-Regame Cloud Gaming Demo - Street Fighter](https://www.bilibili.com/video/bv1jt4y1r7GT) - bilibili

- [01-Regame Cloud Gaming Demo - Street Fighter](https://www.ixigua.com/6898563893564703239) - ixigua

- [01-Regame Cloud Gaming Demo - Street Fighter](https://www.zhihu.com/zvideo/1314567244832530432) - zhihu

## 1. Requirements

### Server

| Target | Minimum | Recommended |
| --- | --- | --- |
| System | Windows 7, 8, 8.1, 10 | Windows 10 |
| GPU | NVIDIA GPU | GTX 1080Ti, RTX 2070S tested |
| Software | GeForce Experience | latest GeForce Experience |
| Driver | GeForce Game Ready Driver | latest GeForce Game Ready Driver |

* If you don't have an NVIDIA GPU, select --enable-nvenc=false to run `cge`.

### Client

| Target | Minimum | Recommended |
| --- | --- | --- |
| System | Windows 7, 8, 8.1, 10 | Windows 10 |
| CPU | ANY | amd64 |

## 2. Basic

Use Hook technology to capture image, support most of DirectX games, such as Cyberpunk 2077, Street Fighter.

[![Data Flow](doc/cg.png)](doc/cg.gv)

## 3. QuickStart

### cge

The `Cloud Gaming Engine`.

You can launch `cge` directly, which will apply default options.

Run `cge --help` to see all options:

```
KSYUN Edge Cloud Gaming Engine v0.3 Beta

Usage:
  -h [ --help ]                  Produce help message
  --audio-bitrate arg (=128000)  Set audio bitrate
  --audio-codec arg (=libopus)   Set audio codec. Select one of {libopus, aac,
                                 opus}
  --bind-address arg (=::)       Set bind address for listening. eg: 0.0.0.0
  --control-port arg (=8080)     Set the UDP port for control flow
  --donot-present arg (=0)       Tell cgh don't present
  --hardware-encoder arg         Set video hardware encoder. Select one of
                                 {amf, nvenc, qsv}
  --keyboard-replay arg (=none)  Set keyboard replay method. Select one of
                                 {none, cgvhid}
  --gamepad-replay arg (=none)   Set gamepad replay method. Select one of
                                 {none, cgvhid, vigem}
  --stream-port arg (=8080)      Set the websocket port for streaming, if port
                                 is 0, disable stream out via network. Capture
                                 and encode picture directly at startup but not
                                 on connection establishing, and never stop
                                 this until cge exit. stream port is not same
                                 as control port, this port is only for media
                                 output.
  --video-bitrate arg (=1000000) Set video bitrate
  --video-codec arg (=h264)      Set video codec. Select one of {h264, h265,
                                 hevc}, h265 == hevc
  --video-gop arg (=180)         Set video gop. [1, 500]
  --video-preset arg             Set preset for video encoder. For AMF, select
                                 one of {speed, balanced, quality}; For NVENC,
                                 select one of {default, slow, medium, fast,
                                 hp, hq, bd, ll, llhq, llhp, lossless,
                                 losslesshp, p1, p2, p3, p4, p5, p6, p7}; For
                                 QSV, select one of {veryfast, faster, fast,
                                 medium, slow, slower, veryslow}; otherwise,
                                 select one of {ultrafast, superfast, veryfast,
                                 faster, fast, medium, slow, slower, veryslow,
                                 placebo}
  --video-quality arg (=23)      Set video quality. [0, 51], lower is better, 0
                                 is lossless
```

You can press `Ctrl+C` to stop it gracefully.

### cgh

Some hook dlls for capturing pictures from D3D game.

### cgi

A tool for injecting hook dll into game process.

[![Hook game](doc/cgi.png)](doc/cgi.gv)

```
Allowed options:
  -h [ --help ]              Produce help message
  -d [ --dynamic ] arg       Use dynamic injecting
  -e [ --exec ] arg          Path of the executable
  -a [ --arg ] arg           Arguments of the executable
  -c [ --cd ] arg            Current directory for the executable
  -i [ --imagename ] arg     Image name of the process being injected.
  -w [ --wait ] arg (=1,000) Wait before injecting. unit: ms
  --lx86 arg                 Path of x86 library path
  --lx64 arg                 Path of x64 library path
```

### cgvhid

Cloud gameing Virtual HID driver. For replaying controller event on server.

![Hook game](doc/cgvhid.png)

### cgvidd

Cloud gameing Virtual Indirect Display Driver. For capturing screen on server.

### video_source

A tool for testing. Just run it, and it will generate simple pictures and write them as video frames to shared memory, then notify `cge` to fetch. You can use `cgc` to see these pictures.

It also can be used to test latency.

Video Reference (Chinese):

- [05-Regame Cloud Gaming Engine: Latency Test](https://www.bilibili.com/video/BV1KU4y147ks/) - bilibili

- [05-Regame Cloud Gaming Engine: Latency Test](https://www.ixigua.com/6918363287298146823) - ixigua

### cgc

A simple client to work with `cge`.

```
Ksyun Edge Cloud Gaming Client v0.3 Beta

Usage:
  -h [ --help ]                         Produce help message
  --audio-frame-delay arg (=2)          Set audio frame max delay, [0, 8]
  -f [ --fullscreen-state ] arg (=none) Set fullscreen state, can be one of
                                        {none, real, fake}
  -l [ --list-hardware-decoder ]        List hardware decoder
  -d [ --hardware-decoder ] arg         Set hardware decoder
  -r [ --remote-host ] arg (=127.0.0.1) Set remote host
  -c [ --control-port ] arg (=8080)     Set remote control port
  -s [ --stream-port ] arg (=8080)      Set remote stream port
  --top-most arg                        Keep the main window always on top
  -u [ --username ] arg                 Set username
  --verification-code arg               Set verification code
  --volume arg (=100)                   Set volume, [0, 100]
```

### cgs

A WebRTC server works together with `cge`, to serve web clients.

## 4. Setting Up Your Build Environment

### 4.1 VS2019

Install VS2019 with ATL, CLang.

[CLang/LLVM x64](https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/LLVM-11.0.0-win64.exe) is also needed, while VS may only has CLang x86.

### 4.2 Boost

Install [Boost](https://www.boost.org/) and set `BOOST_ROOT` environment variable to install directory. [Details](https://blog.umu618.com/2020/09/11/umutech-boost-1-installation/)

Boost compile command:

```
# For MTRelease configuration
.\b2 --address-model=64 runtime-link=static
```

Video Reference (Chinese):

- [06-Regame Cloud Gaming Engine FAQ: Compile Boost](https://www.bilibili.com/video/BV1P5411J7L8/) - bilibili

- [06-Regame Cloud Gaming Engine FAQ: Compile Boost](https://www.ixigua.com/6922104314932986376) - ixigua

### 4.3 FFmpeg

Set `FFMPEG_ROOT` environment variable to the path of your [FFmpeg](https://www.ffmpeg.org/download.html) directory. [LGPL shared is recommended.](https://github.com/BtbN/FFmpeg-Builds/releases)

The folder tree should be like:

```
├─bin
├─include
│  ├─libavcodec
│  ├─libavdevice
│  ├─libavfilter
│  ├─libavformat
│  ├─libavutil
│  ├─libpostproc
│  ├─libswresample
│  └─libswscale
└─lib
    ├─x64
    └─x86
```

Video Reference (Chinese):

- [04-Regame Cloud Gaming Engine FAQ: FFmpeg configuration](https://www.bilibili.com/video/BV1Dh41127xo/) - bilibili

- [04-Regame Cloud Gaming Engine FAQ: FFmpeg configuration](https://www.ixigua.com/6917303228136849933) - ixigua

### 4.4 SDL

Set `SDL2_ROOT` environment variable to the path of your [SDL2](https://www.libsdl.org/) directory. eg:

The same as [SDL_ttf 2.0](https://www.libsdl.org/projects/SDL_ttf/) and [SDL_net 2.0](https://www.libsdl.org/projects/SDL_net/).

The folder tree should be like:

```
├─bin
├─include
│  └─SDL2
└─lib
    ├─x64
    └─x86
```

### 4.5 WDK

Required by cgvhid, cgvidd.

## 5. Building and Testing

```
git clone https://github.com/ksyun-kenc/liuguang
cd liuguang
git submodule update --init
```

Open each solution file with VS, prefer to select MTRelease configuration, then build.

Test steps:

- Run `cge` on server.

- Run `video_source` on server.

- Run `cgc --server=<server_address>` on anther PC as long as it can access the server over the network.

Test with games:

Assume you want to test USF4.

- Install `cgvhid` on server. [Details](src/cgvhid/cgvhid/)

- If you prefer gamepad to keyboard, install `ViGEmBus` on server. [Download](https://github.com/ViGEm/ViGEmBus/releases)

- Run `cge --keyboard-replay=cgvhid --gamepad-replay=vigem` on server.

- Run `cgi -d true -e SSFIV.exe -i SSFIV.exe --lx86 .\captureyuv.dll` on server.

- Run `cgc --server=<server_address>` on anther PC as long as it can access the server over the network.

**Note** that only support D3D9, D3D11, D3D12 games now.

Video Reference (Chinese):

- [02-Regame Cloud Gaming Engine Test Steps](https://www.bilibili.com/video/BV17T4y1N7bk/) - bilibili

- [02-Regame Cloud Gaming Engine Test Steps](https://www.ixigua.com/6909415761098310158) - ixigua

- [02-Regame Cloud Gaming Engine Test Steps](https://www.zhihu.com/zvideo/1325164569828806656) - zhihu

## 6. Contact Us

WeChat: UMUTech

- Please have "regame" or "liuguang" included in friend request.

- Will be more efficient if you could tell me the name of your company.

- Individual develops are also welcome.

## 7. Known Users

<https://github.com/ksyun-kenc/liuguang/wiki/known-users>
