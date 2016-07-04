program sortBubble;

var I, J, TEMP : integer;
var X : array [0 .. 20] of integer;
begin
  for I := 0 to 20 do begin
    X[I] := 20 - I;
  end;
  for I := 0 to 20 do begin
    writeln(X[I]);
  end;
  for I := 1 to 20 do begin
    for J := 20 downto I do begin
      if (X[J] < X[J - 1]) then begin
	TEMP := X[J - 1];
        X[J - 1] := X[J];
	X[J] := TEMP;
      end
    end
  end;
  for I := 0 to 20 do begin
    writeln(X[I]);
  end
end.
