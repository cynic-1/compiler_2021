FROM frolvlad/alpine-gxx
WORKDIR /app/
COPY ./src/lex.cpp ./
RUN g++ lex.cpp -o lex
RUN chmod +x lex