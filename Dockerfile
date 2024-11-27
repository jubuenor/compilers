FROM debian:buster-slim

# Install dependencies

RUN apt-get update && apt-get install -y \
    flex \
    bison \
    build-essential \
    csh \
    openjdk-11-jdk \
    libxaw7-dev \
    wget \
    vim \
    libc6-dev-i386 

# Set the working directory

WORKDIR /usr/class/cool

# Set ownership of the working directory

RUN chown root /usr/class/cool

# Download source code

RUN wget --no-check-certificate 'https://docs.google.com/uc?export=download&id=1Hfxe2c5aqjBs7P9qn0t1rGiMRGX5VBiA' -O student-dist.tar.gz \
 && tar -xf student-dist.tar.gz \
 && rm student-dist.tar.gz

# Set the working directory

WORKDIR /home/root/cool

# Set link

RUN ln -s /usr/class/cool /home/root/cool/

# Set environment variables

ENV PATH="/usr/class/cool/bin:${PATH}"
