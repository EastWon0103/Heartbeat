## Heat 프로그램 (운영체제의 실제 과제)
에러상황에서 처리가 미숙하기 때문에 옵션들이 앞의 가이드라인에 따르지 않으면 오류를 범할 수 있다. 

0. 설치 (Docker 필요)
아래의 명령어들을 실행
```
docker compose up -d --build
docker exec -it work /bin/bash
make clean
make
```

1. 기본 명령어 실행
./heat -i “인터벌 주기(초)”  “curl -sf 테스트 서버 GET 요청"

2. 쉘스크립트로 실행하기
./heat -i “인터벌 주기(초)” -s “검사할 명령어가 담겨있는 쉘 스크립트"

3. 실패를 감지할 경우 프로세스에 시그널 보내기
./heat -i “인터벌 주기(초)” -s “검사할 명령어가 담겨있는 쉘 스크립트" –pid=”PID” –signal=”전달할 시그널"
    - -s 옵션으로 실행해야함
    - signal은 SIGUSR1과 SIGHUP만 실행 가능
    - signal 옵션은 생략될 수 있음, (SIGHUP 시그널 보냄)

4. 실패를 감지할 경우 쉘스크립트 진행
./heat -i 5 -s “검사할 명령어가 담겨있는 쉘 스크립트" –fail=”쉘스크립트"
    - -s 옵션으로 실행해야함
    - fail의 스크립트는 실행 가능해야함

5. 실패를 감지할 경우 복구 쉘스크립트 실행
./heat -i “인터벌 주기(초)” -s 검사할 명령어가 담겨있는 쉘 스크립트" –recovery=”복구 쉘스크립트" –threshold=”실패 누적"
    - -s 옵션으로 실행해야함
    - recovery 쉘스크립트는 실행가능한 쉘스크립트여야만 함
    - recovery.sh를 사용하는 것이 좋음(아니면 -s 옵션에서 실행할 쉘스크립트 설정을 잘 해야함)
