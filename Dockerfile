FROM ubuntu:22.04

RUN mkdir -p /home/user

COPY ./web /home/user/web

EXPOSE 3000

CMD [ "./web" ]