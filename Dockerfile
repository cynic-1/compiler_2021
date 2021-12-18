FROM frolvlad/alpine-gxx
WORKDIR /app/
COPY *.cpp *.h ./
RUN g++ -c *.cpp
RUN g++ *.o -o compiler
RUN chmod +x compiler