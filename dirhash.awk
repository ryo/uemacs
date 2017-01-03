BEGIN {
  count = 0;
}

{
  printf ("%s\n", $0);
}

/^}/ {
  if (count == 0)
    printf ("\nint dir_word_table_top = 0;\n");
  else
    printf ("\nint dir_word_table_end = 0;\n");
  count++;
}
