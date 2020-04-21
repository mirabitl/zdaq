#!/bin/bash
rsync -axv /opt/zdaq/example/lib/ $1:/opt/zdaq/example/lib/
rsync -axv /opt/zdaq/lib/ $1:/opt/zdaq/lib/
rsync -axv /opt/zdaq/bin/ $1:/opt/zdaq/bin/
rsync -axv /opt/zdaq/example/bin/ $1:/opt/zdaq/example/bin/
