FROM arm64v8/debian:buster

WORKDIR /root

RUN apt update
RUN apt install -y libssl1.1

COPY *.deb /root
RUN dpkg -i *.deb
RUN rm *.deb

CMD ["ModbusModule", "/etc/modbusModule/moduleConfiguration.json", "/etc/modbusModule/devicesConfiguration.json"]
