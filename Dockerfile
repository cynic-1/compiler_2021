FROM frolvlad/alpine-gxx
WORKDIR /app/
COPY lex.cpp ./
RUN g++ lex.cpp -o lex
RUN chmod +x lex
