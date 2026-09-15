-----------------다운로드-----------------

1) GLFW
https://www.glfw.org/ 에서Download 로 이동 후, windows pre-compiled binaries 항목에서 64-bit Windows binaries 클릭

2) GLEW for win32
http://glew.sourceforge.net 에서 바이너리 Windows 32비트 및 64비트에서 glew-2.3.1-win32.zip 다운로드 받기
OpenGL 확장Wrangler 라이브러리로 이동한 후 다운로드 된다.

3) GLM
https://glm.g-truc.net/0.9.9/index.html 에서 glm-1.0.3.zip 다운로드 받기
깃허브로 이동한 후 원하는 버전의 라이브러리를 다운로드한다.

-----------------설치방법-----------------

1) include 설치(헤더파일설치)
설치할위치: C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\gl 폴더
설치할파일들:
 1. GLFW 헤더파일들:
    헤더파일 저장된 위치: ..\glfw-3.4.bin.WIN64\glfw-3.4.bin.WIN64\include\GLFW
    저장할 파일들: 폴더 안의 모든 파일들 (glfw3.h, glfw3native.h)
 2. GLEW 헤더파일들: 
    헤더파일 저장된 위치: ..\glew-2.3.1-win32\glew-2.3.1\include\GL
    저장할 파일들:폴더 안의 모든 파일들 (glew.h, glxew.h, wglew.h, eglew.h)
 3. GLM 헤더파일들: 
    헤더파일 저장된 위치:..\glm-1.0.3\glm
    저장할 파일들: glm 디렉토리를 통채로

2) lib 설치(정적라이브러리설치)
설치할위치: C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64 폴더
설치할파일들: *.lib 파일들
 1. GLFW 라이브러리: 
     라이브러리 저장된 위치: ..\glfw-3.4.bin.WIN64\lib-vc2022 폴더
     저장할 파일들: glfw3_mt.lib, glfw3.lib. glfw3dll.lib
 2. GLEW 라이브러리:
     라이브러리 저장된 위치: ..\glew-2.1.0-win32\glew-2.1.0\lib\Release\x64 폴더
     저장할 파일들: glew32.lib, glew32s.lib

3) dll 설치(동적라이브러리설치)
설치할위치: C:\windows\SysWOW64 폴더와C:\windows\system32 폴더
설치할파일들: *.dll파일들
 1. GLFW 라이브러리: 
     라이브러리저장된위치:..\glfw-3.4.bin.WIN64\lib-vc2022
     저장할파일들: glfw3.dll
 2. GLEW 라이브러리:
     라이브러리저장된위치: ..\glew-2.1.0-win32\glew-2.1.0\bin\Release\x64
     저장할파일들: glew32.dll
    
-----------------프로젝트-----------------

1) Visual studio 2026 VC++ 실행
   프로젝트 새로 만들기: 콘솔 응용 프로그램으로 프로젝트 만들기
   Windows 데스크톱 마법사 -> 콘솔애플리케이션 -> 빈프로젝트로 프로젝트 만들기
   프로젝트속성 -> 링커 -> 명령줄에 다음과 같이 3개의 라이브러리 추가
   opengl32.lib glew32.lib glfw3.lib

2) 프로그램에 헤더파일 넣기
   #include <gl/glew.h>
   #include <gl/glfw3.h>
